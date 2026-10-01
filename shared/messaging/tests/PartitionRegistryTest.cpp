//
// Created by DED on 01.10.2026.
//

#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include "PartitionRegistry.h"

using namespace shared::messaging;

namespace {
constexpr auto TOPIC = "orders.created";

PartitionKey key(std::int32_t partition) { return PartitionKey{.topic = TOPIC, .partition = partition}; }

RecordTicket registerOrFail(PartitionRegistry& registry, const PartitionKey& partition, std::int64_t offset) {
    auto ticket = registry.registerRecord(partition, offset);
    EXPECT_TRUE(ticket.has_value());
    return ticket.value();
}
}  // namespace

TEST(PartitionRegistryTest, RegisterOnUnassignedPartitionFails) {
    PartitionRegistry registry;

    const auto result = registry.registerRecord(key(0), 10);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), PartitionRegistryError::PartitionNotAssigned);
}

TEST(PartitionRegistryTest, AssignedPartitionAcceptsRecords) {
    PartitionRegistry registry;
    registry.assign({key(0)});

    EXPECT_TRUE(registry.isAssigned(key(0)));
    EXPECT_TRUE(registry.registerRecord(key(0), 10).has_value());
}

TEST(PartitionRegistryTest, PartitionsProgressIndependently) {
    PartitionRegistry registry;
    registry.assign({key(0), key(1)});

    const auto first = registerOrFail(registry, key(0), 10);
    registerOrFail(registry, key(1), 5);  // на партиции 1 запись не завершена

    ASSERT_EQ(registry.complete(first), CompletionResult::Applied);

    const auto candidates = registry.commitCandidates();
    ASSERT_EQ(candidates.size(), 1);
    EXPECT_EQ(candidates[0].partition, key(0));
    EXPECT_EQ(candidates[0].offset, 11);
}

TEST(PartitionRegistryTest, NoCandidatesWhenNothingCompleted) {
    PartitionRegistry registry;
    registry.assign({key(0)});
    registerOrFail(registry, key(0), 10);

    EXPECT_TRUE(registry.commitCandidates().empty());
}

TEST(PartitionRegistryTest, ConfirmedCommitRemovesCandidate) {
    PartitionRegistry registry;
    registry.assign({key(0)});
    ASSERT_TRUE(registry.complete(registerOrFail(registry, key(0), 10)).has_value());

    const auto candidates = registry.commitCandidates();
    ASSERT_EQ(candidates.size(), 1);

    ASSERT_TRUE(registry.confirmCommit(candidates[0]).has_value());
    EXPECT_TRUE(registry.commitCandidates().empty());
}

TEST(PartitionRegistryTest, UnconfirmedCommitIsOfferedAgain) {
    PartitionRegistry registry;
    registry.assign({key(0)});
    ASSERT_TRUE(registry.complete(registerOrFail(registry, key(0), 10)).has_value());

    EXPECT_EQ(registry.commitCandidates().size(), 1);
    EXPECT_EQ(registry.commitCandidates().size(), 1);
}

TEST(PartitionRegistryTest, RevokeReturnsFinalPositionUpToStuckRecord) {
    PartitionRegistry registry;
    registry.assign({key(0)});

    std::vector<RecordTicket> tickets;
    for (std::int64_t offset = 11; offset <= 15; ++offset)
        tickets.push_back(registerOrFail(registry, key(0), offset));

    for (const auto index : {0, 1, 3, 4})  // 11, 12, 14, 15; запись 13 (индекс 2) не завершена
        ASSERT_TRUE(registry.complete(tickets[index]).has_value());

    const auto finalPositions = registry.revoke({key(0)});

    ASSERT_EQ(finalPositions.size(), 1);
    EXPECT_EQ(finalPositions[0].offset, 13);
}

TEST(PartitionRegistryTest, RevokeRemovesPartition) {
    PartitionRegistry registry;
    registry.assign({key(0)});

    ASSERT_TRUE(registry.revoke({key(0)}).empty());

    EXPECT_FALSE(registry.isAssigned(key(0)));
    EXPECT_FALSE(registry.registerRecord(key(0), 10).has_value());
}

TEST(PartitionRegistryTest, RevokeOfUnknownPartitionIsHarmless) {
    PartitionRegistry registry;

    EXPECT_TRUE(registry.revoke({key(7)}).empty());
}

TEST(PartitionRegistryTest, RevokeAffectsOnlyRequestedPartitions) {
    PartitionRegistry registry;
    registry.assign({key(0), key(1)});

    ASSERT_TRUE(registry.revoke({key(0)}).empty());

    EXPECT_FALSE(registry.isAssigned(key(0)));
    EXPECT_TRUE(registry.isAssigned(key(1)));
}

TEST(PartitionRegistryTest, ReassignStartsFromCleanState) {
    PartitionRegistry registry;
    registry.assign({key(0)});
    registerOrFail(registry, key(0), 100);

    ASSERT_TRUE(registry.revoke({key(0)}).empty());
    registry.assign({key(0)});

    // Трекер без сброса отверг бы оффсет 5 после 100
    EXPECT_TRUE(registry.registerRecord(key(0), 5).has_value());
}

TEST(PartitionRegistryTest, TicketAfterRevokeIsStale) {
    PartitionRegistry registry;
    registry.assign({key(0)});
    const auto ticket = registerOrFail(registry, key(0), 10);

    ASSERT_TRUE(registry.revoke({key(0)}).empty());

    EXPECT_EQ(registry.complete(ticket), CompletionResult::Stale);
}

// Главная защита: партицию забрали и вернули, опоздавший воркер не портит новое состояние
TEST(PartitionRegistryTest, TicketFromPreviousGenerationIsStaleAfterReassign) {
    PartitionRegistry registry;
    registry.assign({key(0)});
    const auto oldTicket = registerOrFail(registry, key(0), 10);

    ASSERT_TRUE(registry.revoke({key(0)}).empty());
    registry.assign({key(0)});
    const auto newTicket = registerOrFail(registry, key(0), 10);  // тот же оффсет, другое поколение

    EXPECT_EQ(registry.complete(oldTicket), CompletionResult::Stale);
    EXPECT_TRUE(registry.commitCandidates().empty());  // старый билет ничего не отметил

    EXPECT_EQ(registry.complete(newTicket), CompletionResult::Applied);
    const auto candidates = registry.commitCandidates();
    ASSERT_EQ(candidates.size(), 1);
    EXPECT_EQ(candidates[0].offset, 11);
}

TEST(PartitionRegistryTest, ConfirmOfStaleCommitIsIgnored) {
    PartitionRegistry registry;
    registry.assign({key(0)});
    ASSERT_TRUE(registry.complete(registerOrFail(registry, key(0), 10)).has_value());
    const auto staleCommit = registry.commitCandidates().front();

    ASSERT_TRUE(registry.revoke({key(0)}).size() == 1);
    registry.assign({key(0)});
    registerOrFail(registry, key(0), 10);

    // Без проверки поколения это дало бы CommitBeyondWatermark в новом трекере
    EXPECT_TRUE(registry.confirmCommit(staleCommit).has_value());
}

TEST(PartitionRegistryTest, CompletingUnknownOffsetPropagatesTrackerError) {
    PartitionRegistry registry;
    registry.assign({key(0)});
    auto ticket = registerOrFail(registry, key(0), 10);
    ticket.offset = 99;

    const auto result = registry.complete(ticket);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), OffsetTrackerError::UnknownOffset);
}

TEST(PartitionRegistryConcurrencyTest, ConcurrentCompletionsLeadToFullCandidate) {
    constexpr int COUNT = 2000;
    constexpr int THREADS = 4;

    PartitionRegistry registry;
    registry.assign({key(0)});

    std::vector<RecordTicket> tickets;
    tickets.reserve(COUNT);
    for (int offset = 0; offset < COUNT; ++offset)
        tickets.push_back(registerOrFail(registry, key(0), offset));

    {
        std::vector<std::jthread> workers;
        for (int thread = 0; thread < THREADS; ++thread) {
            workers.emplace_back([&registry, &tickets, thread] {
                for (int index = thread; index < COUNT; index += THREADS)
                    EXPECT_TRUE(registry.complete(tickets[index]).has_value());
            });
        }
    }  // jthread дожидается завершения в деструкторе

    const auto candidates = registry.commitCandidates();
    ASSERT_EQ(candidates.size(), 1);
    EXPECT_EQ(candidates[0].offset, COUNT);
}

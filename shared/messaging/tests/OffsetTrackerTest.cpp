//
// Created by DED on 30.09.2026.
//

#include <gtest/gtest.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <set>
#include <vector>

#include "OffsetTracker.h"

using namespace shared::messaging;

namespace {
void registerAll(OffsetTracker& tracker, std::initializer_list<std::int64_t> offsets) {
    for (const auto offset : offsets)
        ASSERT_TRUE(tracker.registerRecord(offset).has_value());
}
}  // namespace

TEST(OffsetTrackerTest, EmptyTrackerHasNoCandidate) {
    OffsetTracker tracker;

    EXPECT_EQ(tracker.commitCandidate(), std::nullopt);
    EXPECT_EQ(tracker.uncommittedCount(), 0);
    EXPECT_EQ(tracker.inFlightCount(), 0);
}

TEST(OffsetTrackerTest, SingleRecordGivesCandidateOnlyAfterCompletion) {
    OffsetTracker tracker;
    registerAll(tracker, {10});

    EXPECT_EQ(tracker.commitCandidate(), std::nullopt);

    ASSERT_TRUE(tracker.complete(10).has_value());
    EXPECT_EQ(tracker.commitCandidate(), 11);
}

TEST(OffsetTrackerTest, InOrderCompletionAdvancesCandidateByOne) {
    OffsetTracker tracker;
    registerAll(tracker, {10, 11, 12});

    ASSERT_TRUE(tracker.complete(10).has_value());
    EXPECT_EQ(tracker.commitCandidate(), 11);
    ASSERT_TRUE(tracker.complete(11).has_value());
    EXPECT_EQ(tracker.commitCandidate(), 12);
    ASSERT_TRUE(tracker.complete(12).has_value());
    EXPECT_EQ(tracker.commitCandidate(), 13);
}

TEST(OffsetTrackerTest, ReverseCompletionJumpsOnlyWhenOldestIsDone) {
    OffsetTracker tracker;
    registerAll(tracker, {10, 11, 12});

    ASSERT_TRUE(tracker.complete(12).has_value());
    ASSERT_TRUE(tracker.complete(11).has_value());
    EXPECT_EQ(tracker.commitCandidate(), std::nullopt);

    ASSERT_TRUE(tracker.complete(10).has_value());
    EXPECT_EQ(tracker.commitCandidate(), 13);
}

// Сценарий из обсуждения: 13 не готова - коммитить дальше 13 нельзя
TEST(OffsetTrackerTest, StuckRecordHoldsWatermark) {
    OffsetTracker tracker;
    registerAll(tracker, {11, 12, 13, 14, 15});

    for (const auto offset : {11, 12, 14, 15})
        ASSERT_TRUE(tracker.complete(offset).has_value());

    EXPECT_EQ(tracker.commitCandidate(), 13);

    ASSERT_TRUE(tracker.complete(13).has_value());
    EXPECT_EQ(tracker.commitCandidate(), 16);
}

TEST(OffsetTrackerTest, OutOfOrderCompletionScenario) {
    OffsetTracker tracker;
    registerAll(tracker, {10, 11, 12, 13, 14});

    ASSERT_TRUE(tracker.complete(11).has_value());
    ASSERT_TRUE(tracker.complete(13).has_value());
    EXPECT_EQ(tracker.commitCandidate(), std::nullopt);

    ASSERT_TRUE(tracker.complete(10).has_value());
    EXPECT_EQ(tracker.commitCandidate(), 12);

    ASSERT_TRUE(tracker.complete(12).has_value());
    EXPECT_EQ(tracker.commitCandidate(), 14);

    ASSERT_TRUE(tracker.complete(14).has_value());
    EXPECT_EQ(tracker.commitCandidate(), 15);
}

TEST(OffsetTrackerTest, GapsInOffsetsDoNotBlockWatermark) {
    OffsetTracker tracker;
    registerAll(tracker, {10, 12, 15});

    for (const auto offset : {10, 12, 15})
        ASSERT_TRUE(tracker.complete(offset).has_value());

    EXPECT_EQ(tracker.commitCandidate(), 16);
}

TEST(OffsetTrackerTest, DuplicateCompletionIsHarmless) {
    OffsetTracker tracker;
    registerAll(tracker, {10, 11});

    ASSERT_TRUE(tracker.complete(10).has_value());
    ASSERT_TRUE(tracker.complete(10).has_value());

    EXPECT_EQ(tracker.commitCandidate(), 11);
    EXPECT_EQ(tracker.inFlightCount(), 1);
}

TEST(OffsetTrackerTest, CompletingUnknownOffsetFailsAndKeepsState) {
    OffsetTracker tracker;
    registerAll(tracker, {10, 12});

    const auto result = tracker.complete(11);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), OffsetTrackerError::UnknownOffset);
    EXPECT_EQ(tracker.inFlightCount(), 2);
    EXPECT_EQ(tracker.commitCandidate(), std::nullopt);
}

TEST(OffsetTrackerTest, RegisteringNonIncreasingOffsetFails) {
    OffsetTracker tracker;
    registerAll(tracker, {10, 11});

    for (const auto offset : {11, 10, 5}) {
        const auto result = tracker.registerRecord(offset);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), OffsetTrackerError::OutOfOrderRegistration);
    }
    EXPECT_EQ(tracker.uncommittedCount(), 2);
}

TEST(OffsetTrackerTest, CandidateDisappearsAfterConfirmedCommit) {
    OffsetTracker tracker;
    registerAll(tracker, {10, 11});
    ASSERT_TRUE(tracker.complete(10).has_value());
    ASSERT_TRUE(tracker.complete(11).has_value());

    ASSERT_TRUE(tracker.confirmCommit(12).has_value());

    EXPECT_EQ(tracker.commitCandidate(), std::nullopt);
    EXPECT_EQ(tracker.uncommittedCount(), 0);
}

TEST(OffsetTrackerTest, CandidateStaysWithoutConfirmation) {
    OffsetTracker tracker;
    registerAll(tracker, {10});
    ASSERT_TRUE(tracker.complete(10).has_value());

    // Коммит "упал" - confirmCommit не вызывали, кандидат предлагается снова
    EXPECT_EQ(tracker.commitCandidate(), 11);
    EXPECT_EQ(tracker.commitCandidate(), 11);
}

TEST(OffsetTrackerTest, ConfirmingBeyondWatermarkFails) {
    OffsetTracker tracker;
    registerAll(tracker, {10, 11});
    ASSERT_TRUE(tracker.complete(10).has_value());

    const auto result = tracker.confirmCommit(12);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), OffsetTrackerError::CommitBeyondWatermark);
}

TEST(OffsetTrackerTest, CompletingAlreadyCommittedOffsetIsHarmless) {
    OffsetTracker tracker;
    registerAll(tracker, {10});
    ASSERT_TRUE(tracker.complete(10).has_value());
    ASSERT_TRUE(tracker.confirmCommit(11).has_value());

    EXPECT_TRUE(tracker.complete(10).has_value());
}

TEST(OffsetTrackerTest, CountersTrackWorkAndCommitSeparately) {
    OffsetTracker tracker;
    registerAll(tracker, {10, 11, 12});

    ASSERT_TRUE(tracker.complete(11).has_value());
    EXPECT_EQ(tracker.inFlightCount(), 2);
    EXPECT_EQ(tracker.uncommittedCount(), 3);

    ASSERT_TRUE(tracker.complete(10).has_value());
    EXPECT_EQ(tracker.inFlightCount(), 1);
    EXPECT_EQ(tracker.uncommittedCount(), 3);  // пока не закоммичено, всё ещё "не покрыто"

    ASSERT_TRUE(tracker.confirmCommit(12).has_value());
    EXPECT_EQ(tracker.inFlightCount(), 1);
    EXPECT_EQ(tracker.uncommittedCount(), 1);
}

TEST(OffsetTrackerTest, ResetReturnsTrackerToEmptyStateAndAcceptsAnyOffset) {
    OffsetTracker tracker;
    registerAll(tracker, {10, 11});
    ASSERT_TRUE(tracker.complete(10).has_value());

    tracker.reset();

    EXPECT_EQ(tracker.commitCandidate(), std::nullopt);
    EXPECT_EQ(tracker.uncommittedCount(), 0);
    EXPECT_TRUE(tracker.registerRecord(3).has_value());
}

// Свойство: при любом порядке завершений кандидат не убывает и никогда не обгоняет самую старую незавершённую запись
TEST(OffsetTrackerPropertyTest, CandidateIsMonotonicAndNeverPassesOldestPending) {
    constexpr std::int64_t COUNT = 200;

    std::mt19937 generator(12345);  // фиксированный seed - тест воспроизводим

    for (int round = 0; round < 20; ++round) {
        OffsetTracker tracker;
        std::set<std::int64_t> pending;
        for (std::int64_t offset = 0; offset < COUNT; ++offset) {
            ASSERT_TRUE(tracker.registerRecord(offset).has_value());
            pending.insert(offset);
        }

        std::vector<std::int64_t> order(COUNT);
        std::iota(order.begin(), order.end(), 0);
        std::ranges::shuffle(order, generator);

        std::int64_t previousCandidate = 0;
        for (const auto offset : order) {
            ASSERT_TRUE(tracker.complete(offset).has_value());
            pending.erase(offset);

            const auto candidate = tracker.commitCandidate().value_or(previousCandidate);
            EXPECT_GE(candidate, previousCandidate);
            if (!pending.empty())
                EXPECT_LE(candidate, *pending.begin());
            previousCandidate = candidate;
        }

        EXPECT_EQ(tracker.commitCandidate(), COUNT);
    }
}

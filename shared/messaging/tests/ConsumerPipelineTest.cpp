//
// Created by DED on 02.10.2026.
//

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <format>
#include <future>
#include <map>
#include <mutex>
#include <thread>

#include "ConsumerPipeline.h"
#include "TestLogging.h"

using namespace shared::messaging;
using namespace std::chrono_literals;

namespace {
constexpr auto TOPIC = "orders.created";

PartitionKey key(std::int32_t partition) { return PartitionKey{.topic = TOPIC, .partition = partition}; }

IncomingRecord record(std::int32_t partition, std::int64_t offset, std::string payload) {
    return IncomingRecord{.partition = key(partition),
                          .offset = offset,
                          .payload = std::move(payload),
                          .eventId = std::format("00000000-0000-0000-0000-{:012}", offset),
                          .eventType = "OrderCreated"};
}

template <typename Predicate>
bool waitUntil(Predicate predicate, std::chrono::milliseconds timeout = 5000ms) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!predicate()) {
        if (std::chrono::steady_clock::now() > deadline)
            return false;
        std::this_thread::sleep_for(1ms);
    }
    return true;
}

// Обработчик со сценарием: сколько раз провалить, какие записи удерживать "воротами"
class ScriptedHandler : public IEventHandler {
   public:
    bool handle(const std::string& payload, const MessageMetadata&) override {
        std::shared_future<void> gate;
        {
            std::scoped_lock lock(_mutex);
            ++_attempts[payload];
            if (const auto it = _gates.find(payload); it != _gates.end())
                gate = it->second;
        }
        if (gate.valid())
            gate.wait();

        std::scoped_lock lock(_mutex);
        if (const auto it = _failures.find(payload); it != _failures.end() && it->second != 0) {
            if (it->second > 0)
                --it->second;
            return false;
        }
        ++_completed[payload];
        return true;
    }

    void failTimes(const std::string& payload, int times) {  // -1 = всегда
        std::scoped_lock lock(_mutex);
        _failures[payload] = times;
    }
    void blockUntil(const std::string& payload, std::shared_future<void> gate) {
        std::scoped_lock lock(_mutex);
        _gates[payload] = std::move(gate);
    }
    int attempts(const std::string& payload) const {
        std::scoped_lock lock(_mutex);
        const auto it = _attempts.find(payload);
        return it == _attempts.end() ? 0 : it->second;
    }
    bool completed(const std::string& payload) const {
        std::scoped_lock lock(_mutex);
        return _completed.contains(payload);
    }

   private:
    mutable std::mutex _mutex;
    std::map<std::string, int> _failures;
    std::map<std::string, std::shared_future<void>> _gates;
    std::map<std::string, int> _attempts;
    std::map<std::string, int> _completed;
};

class FakeSink : public IDeadLetterSink {
   public:
    std::expected<void, std::error_code> send(const DeadLetterRecord& record) override {
        std::scoped_lock lock(_mutex);
        _payloads.push_back(record.payload);
        _attempts.push_back(record.attempts);
        return {};
    }
    std::vector<std::string> payloads() const {
        std::scoped_lock lock(_mutex);
        return _payloads;
    }
    std::vector<int> attempts() const {
        std::scoped_lock lock(_mutex);
        return _attempts;
    }

   private:
    mutable std::mutex _mutex;
    std::vector<std::string> _payloads;
    std::vector<int> _attempts;
};
}  // namespace

class ConsumerPipelineTest : public ::testing::Test {
   protected:
    void SetUp() override { _gate = _release.get_future().share(); }

    // Воркер, висящий на воротах, не даст пулу завершиться: отпускаем  до уничтожения pipeline
    void TearDown() override {
        releaseGate();
        _pipeline.reset();
    }

    void releaseGate() {
        if (!_released.exchange(true))
            _release.set_value();
    }

    void makePipeline(std::size_t threads = 2, std::size_t queue = 8, int maxAttempts = 3) {
        _pipeline = std::make_unique<ConsumerPipeline>(
            _handler, _sink,
            PipelineConfiguration{.workerThreads = threads,
                           .queueCapacity = queue,
                           .retry = RetryPolicy{.maxAttempts = maxAttempts,
                                                .initialBackoff = 1ms,
                                                .multiplier = 1.0,
                                                .maxBackoff = 1ms}});
        _pipeline->onPartitionsAssigned({key(0)});
    }

    std::optional<std::int64_t> candidate(std::int32_t partition = 0) const {
        for (const auto& position : _pipeline->commitCandidates()) {
            if (position.partition.partition == partition)
                return position.offset;
        }
        return std::nullopt;
    }

    ScriptedHandler _handler;
    FakeSink _sink;
    std::promise<void> _release;
    std::shared_future<void> _gate;
    std::atomic<bool> _released{false};
    std::unique_ptr<ConsumerPipeline> _pipeline;  // последним
};

TEST_F(ConsumerPipelineTest, HealthyRecordsBecomeCommittable) {
    makePipeline();
    for (std::int64_t offset = 10; offset <= 12; ++offset)
        _pipeline->ingest(record(0, offset, std::format("m{}", offset)));

    EXPECT_TRUE(_pipeline->pump());

    ASSERT_TRUE(waitUntil([&] { return candidate() == 13; }));
}

TEST_F(ConsumerPipelineTest, FailedRecordIsRetriedInPlaceWithoutRetryingOthers) {
    makePipeline();
    _handler.failTimes("m10", 1);
    for (std::int64_t offset = 10; offset <= 12; ++offset)
        _pipeline->ingest(record(0, offset, std::format("m{}", offset)));

    EXPECT_TRUE(_pipeline->pump());

    ASSERT_TRUE(waitUntil([&] { return candidate() == 13; }));
    EXPECT_EQ(_handler.attempts("m10"), 2);
    EXPECT_EQ(_handler.attempts("m11"), 1);
    EXPECT_EQ(_handler.attempts("m12"), 1);
}

TEST_F(ConsumerPipelineTest, PoisonRecordGoesToDeadLetterAndDoesNotBlockCommit) {
    makePipeline(2, 8, 3);
    _handler.failTimes("m10", -1);
    _pipeline->ingest(record(0, 10, "m10"));
    _pipeline->ingest(record(0, 11, "m11"));

    EXPECT_TRUE(_pipeline->pump());

    ASSERT_TRUE(waitUntil([&] { return candidate() == 12; }));
    EXPECT_EQ(_sink.payloads(), std::vector<std::string>{"m10"});
    EXPECT_EQ(_sink.attempts(), std::vector<int>{3});
}

TEST_F(ConsumerPipelineTest, StuckRecordHoldsCommitUntilItCompletes) {
    makePipeline();
    _handler.blockUntil("m10", _gate);
    for (std::int64_t offset = 10; offset <= 12; ++offset)
        _pipeline->ingest(record(0, offset, std::format("m{}", offset)));

    EXPECT_TRUE(_pipeline->pump());

    ASSERT_TRUE(waitUntil([&] { return _handler.completed("m11") && _handler.completed("m12"); }));
    EXPECT_EQ(candidate(), std::nullopt);

    releaseGate();
    ASSERT_TRUE(waitUntil([&] { return candidate() == 13; }));
}

TEST_F(ConsumerPipelineTest, FullPoolKeepsRecordsInBacklogUntilItDrains) {
    makePipeline(1, 1);
    _handler.blockUntil("m10", _gate);
    for (std::int64_t offset = 10; offset <= 13; ++offset)
        _pipeline->ingest(record(0, offset, std::format("m{}", offset)));

    // 1 поток + очередь на 1 => максимум 2 записи в системе, минимум 2 остаются в backlog
    EXPECT_FALSE(_pipeline->pump());
    EXPECT_GE(_pipeline->backlogSize(), 2);

    releaseGate();
    ASSERT_TRUE(waitUntil([&] { return _pipeline->pump(); }));
    ASSERT_TRUE(waitUntil([&] { return candidate() == 14; }));
}

TEST_F(ConsumerPipelineTest, RevokeReturnsFinalPositionUpToStuckRecord) {
    makePipeline();
    _handler.blockUntil("m11", _gate);
    _pipeline->ingest(record(0, 10, "m10"));
    _pipeline->ingest(record(0, 11, "m11"));
    EXPECT_TRUE(_pipeline->pump());
    ASSERT_TRUE(waitUntil([&] { return candidate() == 11; }));

    const auto positions = _pipeline->onPartitionsRevoked({key(0)});

    ASSERT_EQ(positions.size(), 1);
    EXPECT_EQ(positions[0].offset, 11);
}

TEST_F(ConsumerPipelineTest, RevokeDropsBacklogOfRevokedPartitions) {
    makePipeline(1, 1);
    _handler.blockUntil("m10", _gate);
    for (std::int64_t offset = 10; offset <= 13; ++offset)
        _pipeline->ingest(record(0, offset, std::format("m{}", offset)));
    EXPECT_FALSE(_pipeline->pump());
    ASSERT_GE(_pipeline->backlogSize(), 2);

    std::ignore = _pipeline->onPartitionsRevoked({key(0)});

    EXPECT_EQ(_pipeline->backlogSize(), 0);
}

TEST_F(ConsumerPipelineTest, RecordWithInvalidEventIdIsSkippedAndDoesNotBlockCommit) {
    makePipeline();
    auto broken = record(0, 10, "bad");
    broken.eventId = "not-a-uuid";

    _pipeline->ingest(std::move(broken));

    EXPECT_EQ(candidate(), 11);
    EXPECT_EQ(_handler.attempts("bad"), 0);
}

TEST_F(ConsumerPipelineTest, RecordWithoutEventIdIsSkipped) {
    makePipeline();
    auto broken = record(0, 10, "bad");
    broken.eventId = std::nullopt;

    _pipeline->ingest(std::move(broken));

    EXPECT_EQ(candidate(), 11);
    EXPECT_EQ(_handler.attempts("bad"), 0);
}

TEST_F(ConsumerPipelineTest, RecordFromUnassignedPartitionIsIgnored) {
    makePipeline();

    _pipeline->ingest(record(5, 10, "m"));

    EXPECT_TRUE(_pipeline->pump());
    EXPECT_EQ(_pipeline->backlogSize(), 0);
    EXPECT_TRUE(_pipeline->commitCandidates().empty());
    EXPECT_EQ(_handler.attempts("m"), 0);
}

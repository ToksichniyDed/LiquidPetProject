//
// Created by DED on 02.10.2026.
//

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

#include <logging/Logger.h>

#include "PublisherDeadLetterSink.h"
#include "RecordExecutor.h"

using namespace shared::messaging;
using namespace std::chrono_literals;
using ::testing::_;
using ::testing::AllOf;
using ::testing::Field;
using ::testing::Return;
using ::testing::Throw;

namespace {
constexpr auto TOPIC = "orders.created";
constexpr auto EVENT_ID = "44444444-4444-4444-4444-444444444444";

PartitionKey key(std::int32_t partition) { return PartitionKey{.topic = TOPIC, .partition = partition}; }

class MockEventHandler : public IEventHandler {
   public:
    MOCK_METHOD(bool, handle, (const std::string& payload, const MessageMetadata& metadata), (override));
};

class MockDeadLetterSink : public IDeadLetterSink {
   public:
    MOCK_METHOD((std::expected<void, std::error_code>), send, (const DeadLetterRecord& record), (override));
};

std::expected<void, std::error_code> ok() { return {}; }
std::expected<void, std::error_code> brokerRejected() {
    return std::unexpected(EventPublisherError::BrokerRejected);
}
}  // namespace

class RecordExecutorTest : public ::testing::Test {
   protected:
    static void SetUpTestSuite() { shared::logger::init(true, false, spdlog::level::debug, {}, 1024, 0); }

    void SetUp() override {
        _registry.assign({key(0)});
        _ticket = _registry.registerRecord(key(0), 10).value();
    }

    [[nodiscard]] WorkItem item() const {
        return WorkItem{.ticket = _ticket,
                        .payload = R"({"orderId":"x"})",
                        .metadata = MessageMetadata{.eventId = shared::models::OutboxEventId::create(EVENT_ID).value(),
                                                    .eventType = "OrderCreated"}};
    }

    void expectCompleted() {
        const auto candidates = _registry.commitCandidates();
        ASSERT_EQ(candidates.size(), 1);
        EXPECT_EQ(candidates[0].offset, 11);
    }

    void expectNotCompleted() { EXPECT_TRUE(_registry.commitCandidates().empty()); }

    MockEventHandler _handler;
    MockDeadLetterSink _sink;
    PartitionRegistry _registry;
    RecordTicket _ticket{};
    RetryPolicy _policy{.maxAttempts = 3, .initialBackoff = 1ms, .multiplier = 1.0, .maxBackoff = 1ms};
    RecordExecutor _executor{_handler, _sink, _registry, _policy};
    std::stop_source _stop;
};

TEST_F(RecordExecutorTest, SucceedsOnFirstAttempt) {
    EXPECT_CALL(_handler, handle(_, _)).WillOnce(Return(true));
    EXPECT_CALL(_sink, send(_)).Times(0);

    EXPECT_EQ(_executor.execute(item(), _stop.get_token()), RecordOutcome::Succeeded);
    expectCompleted();
}

TEST_F(RecordExecutorTest, RetriesUntilHandlerSucceeds) {
    EXPECT_CALL(_handler, handle(_, _)).WillOnce(Return(false)).WillOnce(Return(false)).WillOnce(Return(true));
    EXPECT_CALL(_sink, send(_)).Times(0);

    EXPECT_EQ(_executor.execute(item(), _stop.get_token()), RecordOutcome::Succeeded);
    expectCompleted();
}

TEST_F(RecordExecutorTest, HandlerExceptionCountsAsFailedAttempt) {
    EXPECT_CALL(_handler, handle(_, _)).WillOnce(Throw(std::runtime_error("boom"))).WillOnce(Return(true));

    EXPECT_EQ(_executor.execute(item(), _stop.get_token()), RecordOutcome::Succeeded);
    expectCompleted();
}

TEST_F(RecordExecutorTest, ExhaustedRetriesGoToDeadLetterWithOriginalPayload) {
    EXPECT_CALL(_handler, handle(_, _)).Times(3).WillRepeatedly(Return(false));
    EXPECT_CALL(_sink, send(AllOf(Field(&DeadLetterRecord::attempts, 3), Field(&DeadLetterRecord::offset, 10),
                                  Field(&DeadLetterRecord::payload, std::string{R"({"orderId":"x"})"}))))
        .WillOnce(Return(ok()));

    EXPECT_EQ(_executor.execute(item(), _stop.get_token()), RecordOutcome::DeadLettered);
    expectCompleted();
}

TEST_F(RecordExecutorTest, DeadLetterSendIsRetriedUntilItSucceeds) {
    EXPECT_CALL(_handler, handle(_, _)).Times(3).WillRepeatedly(Return(false));
    EXPECT_CALL(_sink, send(_)).WillOnce(Return(brokerRejected())).WillOnce(Return(ok()));

    EXPECT_EQ(_executor.execute(item(), _stop.get_token()), RecordOutcome::DeadLettered);
    expectCompleted();
}

TEST_F(RecordExecutorTest, UnavailableDeadLetterWithStopLeavesRecordIncomplete) {
    EXPECT_CALL(_handler, handle(_, _)).Times(3).WillRepeatedly(Return(false));
    EXPECT_CALL(_sink, send(_)).WillRepeatedly([this](const DeadLetterRecord&) {
        _stop.request_stop();
        return brokerRejected();
    });

    EXPECT_EQ(_executor.execute(item(), _stop.get_token()), RecordOutcome::Abandoned);
    expectNotCompleted();
}

TEST_F(RecordExecutorTest, AlreadyStoppedAbandonsWithoutCallingHandler) {
    EXPECT_CALL(_handler, handle(_, _)).Times(0);
    _stop.request_stop();

    EXPECT_EQ(_executor.execute(item(), _stop.get_token()), RecordOutcome::Abandoned);
    expectNotCompleted();
}

TEST_F(RecordExecutorTest, RevokedPartitionStopsRetrying) {
    // Первая попытка падает, и в этот момент партицию отбирают
    EXPECT_CALL(_handler, handle(_, _)).WillOnce([this](const std::string&, const MessageMetadata&) {
        std::ignore = _registry.revoke({key(0)});
        return false;
    });
    EXPECT_CALL(_sink, send(_)).Times(0);

    EXPECT_EQ(_executor.execute(item(), _stop.get_token()), RecordOutcome::Abandoned);
}

namespace {
class MockPublisher : public IEventPublisher {
   public:
    MOCK_METHOD((std::future<std::expected<void, std::error_code>>), publish, (const PublishRequest& request),
                (override));
};

std::future<std::expected<void, std::error_code>> readyFuture(std::expected<void, std::error_code> value) {
    std::promise<std::expected<void, std::error_code>> promise;
    promise.set_value(std::move(value));
    return promise.get_future();
}

DeadLetterRecord sampleRecord() {
    return DeadLetterRecord{.source = key(2),
                            .offset = 42,
                            .attempts = 5,
                            .payload = "payload",
                            .metadata = MessageMetadata{
                                .eventId = shared::models::OutboxEventId::create(EVENT_ID).value(),
                                .eventType = "OrderCreated"}};
}
}  // namespace

TEST(PublisherDeadLetterSinkTest, PublishesToDlqTopicKeepingPayloadAndMetadata) {
    auto publisher = std::make_shared<MockPublisher>();
    const auto record = sampleRecord();
    EXPECT_CALL(*publisher, publish(PublishRequest{.topic = "orders.created.dlq",
                                                   .key = EVENT_ID,
                                                   .payload = "payload",
                                                   .metadata = record.metadata}))
        .WillOnce([](const PublishRequest&) { return readyFuture(ok()); });

    PublisherDeadLetterSink sink(publisher);

    EXPECT_TRUE(sink.send(record).has_value());
}

TEST(PublisherDeadLetterSinkTest, PropagatesPublisherError) {
    auto publisher = std::make_shared<MockPublisher>();
    EXPECT_CALL(*publisher, publish(_)).WillOnce([](const PublishRequest&) { return readyFuture(brokerRejected()); });

    PublisherDeadLetterSink sink(publisher);
    const auto result = sink.send(sampleRecord());

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), EventPublisherError::BrokerRejected);
}

TEST(PublisherDeadLetterSinkTest, ReturnsTimeoutWhenPublishDoesNotComplete) {
    auto publisher = std::make_shared<MockPublisher>();
    std::promise<std::expected<void, std::error_code>> neverSet;
    EXPECT_CALL(*publisher, publish(_)).WillOnce([&neverSet](const PublishRequest&) { return neverSet.get_future(); });

    PublisherDeadLetterSink sink(publisher, ".dlq", 10ms);
    const auto result = sink.send(sampleRecord());

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), EventPublisherError::Timeout);
}

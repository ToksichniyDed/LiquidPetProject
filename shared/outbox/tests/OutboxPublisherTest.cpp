//
// Created by DED on 06.09.2026.
//

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <future>

#include <OutboxPublisher.h>
#include "MockEventPublisher.h"
#include "MockOutboxRepository.h"

namespace order_service::outbox {

    using ::testing::_;
    using ::testing::Return;

    using shared::outbox::MockOutboxRepository;
    using shared::outbox::OutboxEntry;
    using shared::messaging::MockEventPublisher;
    using shared::messaging::makeReadyFuture;
    using shared::messaging::PublishRequest;
    using shared::messaging::MessageMetadata;

    class OutboxPublisherTest : public ::testing::Test {
    protected:
        void SetUp() override {
            _repository = std::make_shared<MockOutboxRepository>();
            _publisher = std::make_shared<MockEventPublisher>();
        }

        static constexpr auto kFastPollInterval = std::chrono::milliseconds(10);
        static constexpr auto kTopic = "orders.events";

        std::shared_ptr<MockOutboxRepository> _repository;
        std::shared_ptr<MockEventPublisher> _publisher;
    };

    TEST_F(OutboxPublisherTest, PublishesUnpublishedEntryAndMarksItPublished) {
        const OutboxEntry entry{
            .id = shared::models::OutboxEventId::create("11111111-1111-1111-1111-111111111111").value(),
            .aggregateId = "order-1",
            .eventType = "OrderCreated",
            .payload = R"({"orderId":"order-1"})"
        };

        std::promise<void> markedAsPublished;
        auto markedAsPublishedFuture = markedAsPublished.get_future();

        EXPECT_CALL(*_repository, fetchUnpublished(_))
            .WillOnce(Return(std::vector<OutboxEntry>{entry}))
            .WillRepeatedly(Return(std::vector<OutboxEntry>{}));

        const PublishRequest expectedRequest{
            .topic = kTopic,
            .key = entry.aggregateId,
            .payload = entry.payload,
            .metadata = MessageMetadata{.eventId = entry.id, .eventType = entry.eventType},
        };

        EXPECT_CALL(*_publisher, publish(::testing::Eq(expectedRequest)))
            .WillOnce([](const PublishRequest&) { return makeReadyFuture({}); });

        EXPECT_CALL(*_repository, markAsPublished(entry.id))
            .WillOnce([&markedAsPublished](const shared::models::OutboxEventId&) {
                markedAsPublished.set_value();
                return std::expected<void, std::error_code>{};
            });

        shared::outbox::OutboxPublisher publisher(_repository, _publisher, kTopic, kFastPollInterval);
        publisher.start();

        const auto status = markedAsPublishedFuture.wait_for(std::chrono::seconds(2));
        publisher.stop();

        ASSERT_EQ(status, std::future_status::ready) << "markAsPublished was not called in time";
    }

    TEST_F(OutboxPublisherTest, DoesNotMarkAsPublishedWhenPublishFails) {
        const OutboxEntry entry{
            .id = shared::models::OutboxEventId::create("22222222-2222-2222-2222-222222222222").value(),
            .aggregateId = "order-2",
            .eventType = "OrderCreated",
            .payload = R"({"orderId":"order-2"})"
        };

        std::promise<void> publishAttempted;
        auto publishAttemptedFuture = publishAttempted.get_future();

        EXPECT_CALL(*_repository, fetchUnpublished(_))
            .WillOnce(Return(std::vector<OutboxEntry>{entry}))
            .WillRepeatedly(Return(std::vector<OutboxEntry>{}));

        EXPECT_CALL(*_publisher, publish(_))
            .WillOnce([&publishAttempted](const PublishRequest&) {
                publishAttempted.set_value();
                return makeReadyFuture(std::expected<void, std::error_code>{
                    std::unexpected(shared::messaging::EventPublisherError::BrokerRejected)});
            });

        // раз публикация не удалась, markAsPublished не должен вызываться вообще
        EXPECT_CALL(*_repository, markAsPublished(_)).Times(0);

        shared::outbox::OutboxPublisher publisher(_repository, _publisher, kTopic, kFastPollInterval);
        publisher.start();

        const auto status = publishAttemptedFuture.wait_for(std::chrono::seconds(2));
        publisher.stop();

        ASSERT_EQ(status, std::future_status::ready) << "publish was not called in time";
    }

    TEST_F(OutboxPublisherTest, ContinuesPollingWhenFetchFails) {
        std::promise<void> fetchedAtLeastTwice;
        auto fetchedFuture = fetchedAtLeastTwice.get_future();
        std::atomic<int> callCount{0};

        EXPECT_CALL(*_repository, fetchUnpublished(_))
            .WillRepeatedly([&callCount, &fetchedAtLeastTwice](int) {
                if (++callCount == 2) {
                    fetchedAtLeastTwice.set_value();
                }
                return std::expected<std::vector<OutboxEntry>, std::error_code>{
                    std::unexpected(shared::outbox::OutboxRepositoryError::ConnectionFailure)};
            });

        EXPECT_CALL(*_publisher, publish(_)).Times(0);

        shared::outbox::OutboxPublisher publisher(_repository, _publisher, kTopic, kFastPollInterval);
        publisher.start();

        const auto status = fetchedFuture.wait_for(std::chrono::seconds(2));
        publisher.stop();

        ASSERT_EQ(status, std::future_status::ready) << "publisher did not keep polling after fetch failure";
    }

    TEST_F(OutboxPublisherTest, PublishesAllEntriesInBatchConcurrentlyBeforeAwaitingResults) {
        const OutboxEntry entryA{
            .id = shared::models::OutboxEventId::create("33333333-3333-3333-3333-333333333333").value(),
            .aggregateId = "order-a",
            .eventType = "OrderCreated",
            .payload = "{}"
        };
        const OutboxEntry entryB{
            .id = shared::models::OutboxEventId::create("44444444-4444-4444-4444-444444444444").value(),
            .aggregateId = "order-b",
            .eventType = "OrderCreated",
            .payload = "{}"
        };

        std::promise<std::expected<void, std::error_code>> promiseA;
        std::promise<std::expected<void, std::error_code>> promiseB;

        std::promise<void> bothPublishCalled;
        auto bothPublishCalledFuture = bothPublishCalled.get_future();
        std::atomic<int> publishCallCount{0};

        EXPECT_CALL(*_repository, fetchUnpublished(_))
            .WillOnce(Return(std::vector<OutboxEntry>{entryA, entryB}))
            .WillRepeatedly(Return(std::vector<OutboxEntry>{}));

        EXPECT_CALL(*_publisher, publish(::testing::Field(&PublishRequest::key, "order-a")))
            .WillOnce([&publishCallCount, &bothPublishCalled, &promiseA](const PublishRequest&) {
                if (++publishCallCount == 2) bothPublishCalled.set_value();
                return promiseA.get_future();
            });

        EXPECT_CALL(*_publisher, publish(::testing::Field(&PublishRequest::key, "order-b")))
            .WillOnce([&publishCallCount, &bothPublishCalled, &promiseB](const PublishRequest&) {
                if (++publishCallCount == 2) bothPublishCalled.set_value();
                return promiseB.get_future();
            });

        EXPECT_CALL(*_repository, markAsPublished(_))
            .WillRepeatedly(Return(std::expected<void, std::error_code>{}));

        shared::outbox::OutboxPublisher publisher(_repository, _publisher, kTopic, kFastPollInterval);
        publisher.start();

        const auto status = bothPublishCalledFuture.wait_for(std::chrono::seconds(2));
        ASSERT_EQ(status, std::future_status::ready)
            << "publish was not called for both entries before awaiting results - batching is broken";

        promiseA.set_value({});
        promiseB.set_value({});

        publisher.stop();
    }

}

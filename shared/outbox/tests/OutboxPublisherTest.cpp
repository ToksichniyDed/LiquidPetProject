//
// Created by DED on 06.09.2026.
//

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <future>

#include "../../../shared/outbox/OutboxPublisher.h"
#include "MockEventPublisher.h"
#include "MockOutboxRepository.h"

namespace order_service::outbox {

    using ::testing::_;
    using ::testing::Return;

    using outbox::MockOutboxRepository;
    using outbox::OutboxEntry;
    using messaging::MockEventPublisher;
    using messaging::makeReadyFuture;

    class OutboxPublisherTest : public ::testing::Test {
    protected:
        void SetUp() override {
            _repository = std::make_shared<MockOutboxRepository>();
            _publisher = std::make_shared<MockEventPublisher>();
        }

        static constexpr auto kFastPollInterval = std::chrono::milliseconds(10);

        std::shared_ptr<MockOutboxRepository> _repository;
        std::shared_ptr<MockEventPublisher> _publisher;
    };

    TEST_F(OutboxPublisherTest, PublishesUnpublishedEntryAndMarksItPublished) {
        const OutboxEntry entry{
            .id = "outbox-1",
            .aggregateId = "order-1",
            .eventType = "OrderCreated",
            .payload = R"({"orderId":"order-1"})"
        };

        std::promise<void> markedAsPublished;
        auto markedAsPublishedFuture = markedAsPublished.get_future();

        EXPECT_CALL(*_repository, fetchUnpublished(_))
            .WillOnce(Return(std::vector<OutboxEntry>{entry}))
            .WillRepeatedly(Return(std::vector<OutboxEntry>{}));

        EXPECT_CALL(*_publisher, publish("orders.events", entry.aggregateId, entry.payload))
            .WillOnce([](const std::string&, const std::string&, const std::string&) {
                                                                                                return makeReadyFuture(
                                                                                                    {});
                                                                                            });

        EXPECT_CALL(*_repository, markAsPublished(entry.id))
            .WillOnce([&markedAsPublished](const std::string&) {
                markedAsPublished.set_value();
                return std::expected<void, std::error_code>{};
        });

        OutboxPublisher publisher(_repository, _publisher, "orders.events", kFastPollInterval);
        publisher.start();

        const auto status = markedAsPublishedFuture.wait_for(std::chrono::seconds(2));
        publisher.stop();

        ASSERT_EQ(status, std::future_status::ready) << "markAsPublished was not called in time";
    }

    TEST_F(OutboxPublisherTest, DoesNotMarkAsPublishedWhenPublishFails) {
        const OutboxEntry entry{
            .id = "outbox-2",
            .aggregateId = "order-2",
            .eventType = "OrderCreated",
            .payload = R"({"orderId":"order-2"})"
        };

        std::promise<void> publishAttempted;
        auto publishAttemptedFuture = publishAttempted.get_future();

        EXPECT_CALL(*_repository, fetchUnpublished(_))
            .WillOnce(Return(std::vector<OutboxEntry>{entry}))
            .WillRepeatedly(Return(std::vector<OutboxEntry>{}));

        EXPECT_CALL(*_publisher, publish(_, _, _))
            .WillOnce([&publishAttempted](const std::string&, const std::string&, const std::string&) {
                                                      publishAttempted.set_value();
                                                      return makeReadyFuture(
                                                          std::expected<void, std::error_code>{
                                                              std::unexpected(
                                                                  messaging::EventPublisherError::BrokerRejected)
                                                          });
        });

        // раз публикация не удалась, markAsPublished не должен вызываться вообще
        EXPECT_CALL(*_repository, markAsPublished(_)).Times(0);

        OutboxPublisher publisher(_repository, _publisher, "orders.events", kFastPollInterval);
        publisher.start();

        const auto status = publishAttemptedFuture.wait_for(std::chrono::seconds(2));
        publisher.stop();

        ASSERT_EQ(status, std::future_status::ready) << "publish was not called in time";
    }

    TEST_F(OutboxPublisherTest, ContinuesPollingWhenFetchFails) {
        std::promise<void> fetchedAtLeastTwice;
        auto fetchedFuture = fetchedAtLeastTwice.get_future();
        std::atomic<int> callCount{0};

        // repository падает с ошибкой, но цикл не должен падать/останавливаться
        // должен просто попробовать снова в следующем polling-цикле
        EXPECT_CALL(*_repository, fetchUnpublished(_))
            .WillRepeatedly([&callCount, &fetchedAtLeastTwice](int) {
                                                          if (++callCount == 2) {
                                                              fetchedAtLeastTwice.set_value();
                                                          }
                                                          return std::expected<
                                                              std::vector<OutboxEntry>, std::error_code>{
                                                              std::unexpected(OutboxRepositoryError::ConnectionFailure)
                                                          };
                                                      });

        EXPECT_CALL(*_publisher, publish(_, _, _)).Times(0);

        OutboxPublisher publisher(_repository, _publisher, "orders.events", kFastPollInterval);
        publisher.start();

        const auto status = fetchedFuture.wait_for(std::chrono::seconds(2));
        publisher.stop();

        ASSERT_EQ(status, std::future_status::ready) << "publisher did not keep polling after fetch failure";
    }

    TEST_F(OutboxPublisherTest, PublishesAllEntriesInBatchConcurrentlyBeforeAwaitingResults) {
        // publish() должен быть вызван для каждой записи
        // батча до того, как паблишер начнёт дожидаться результатов
        // имитируем это тем, что все promise выставляются позже, вручную,
        // а не сразу внутри лямбды publish()
        const OutboxEntry entryA{.id = "a", .aggregateId = "order-a", .eventType = "OrderCreated", .payload = "{}"};
        const OutboxEntry entryB{.id = "b", .aggregateId = "order-b", .eventType = "OrderCreated", .payload = "{}"};

        std::promise<std::expected<void, std::error_code>> promiseA;
        std::promise<std::expected<void, std::error_code>> promiseB;

        std::promise<void> bothPublishCalled;
        auto bothPublishCalledFuture = bothPublishCalled.get_future();
        std::atomic<int> publishCallCount{0};

        EXPECT_CALL(*_repository, fetchUnpublished(_))
            .WillOnce(Return(std::vector<OutboxEntry>{entryA, entryB}))
            .WillRepeatedly(Return(std::vector<OutboxEntry>{}));

        EXPECT_CALL(*_publisher, publish(_, "order-a", _))
            .WillOnce([&publishCallCount, &bothPublishCalled, &promiseA](
                                                          const std::string&, const std::string&, const std::string&) {
                                                                  if (++publishCallCount == 2)
                                                                      bothPublishCalled.set_value();
                                                                  return promiseA.get_future();
                                                              });

        EXPECT_CALL(*_publisher, publish(_, "order-b", _))
            .WillOnce([&publishCallCount, &bothPublishCalled, &promiseB](
                                                          const std::string&, const std::string&, const std::string&) {
                                                                  if (++publishCallCount == 2)
                                                                      bothPublishCalled.set_value();
                                                                  return promiseB.get_future();
                                                              });

        EXPECT_CALL(*_repository, markAsPublished(_))
            .WillRepeatedly(Return(std::expected<void, std::error_code>{}));

        OutboxPublisher publisher(_repository, _publisher, "orders.events", kFastPollInterval);
        publisher.start();

        const auto status = bothPublishCalledFuture.wait_for(std::chrono::seconds(2));
        ASSERT_EQ(status, std::future_status::ready)
            << "publish was not called for both entries before awaiting results - batching is broken";

        promiseA.set_value({});
        promiseB.set_value({});

        publisher.stop();
    }

}

//
// Created by DED on 06.09.2026.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <future>
#include <chrono>

#include "OutboxPublisher.h"
#include "MockOutboxRepository.h"
#include "MockEventPublisher.h"

namespace order_service::outbox {

    using ::testing::_;
    using ::testing::Return;
    using ::testing::Invoke;

    using outbox::MockOutboxRepository;
    using outbox::OutboxEntry;
    using messaging::MockEventPublisher;

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

        // первый вызов отдаёт запись, все последующие - пустой список,
        // чтобы цикл не пытался переотправить её бесконечно
        EXPECT_CALL(*_repository, fetchUnpublished(_))
            .WillOnce(Return(std::vector<OutboxEntry>{entry}))
            .WillRepeatedly(Return(std::vector<OutboxEntry>{}));

        EXPECT_CALL(*_publisher, publish("orders.events", entry.aggregateId, entry.payload))
                .WillOnce(Return(std::expected<void, std::error_code>{}));

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
                                                      return std::expected<void, std::error_code>{
                                                          std::unexpected(
                                                              messaging::EventPublisherError::BrokerRejected)
                                                      };
                                                  });

        //раз публикация не удалась - markAsPublished не должен вызываться вообще
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

        // repository падает с ошибкой, но цикл не должен падать/останавливаться -
        // должен просто попробовать снова в следующем polling-цикле
        EXPECT_CALL(*_repository, fetchUnpublished(_))
            .WillRepeatedly([&](int) {
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

}

//
// Created by DED on 13.09.2026.
//

#include <OrderReservedEvent.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <mapper/OrderCreatedEventJsonMapper.h>
#include <mapper/OrderReservedEventJsonMapper.h>

#include <nlohmann/json.hpp>

#include "MockOrderProcessor.h"
#include "MockWorkerRepository.h"
#include "OrderReservationHandler.h"

using namespace worker_service::handlers;
using namespace worker_service::repository;
using namespace worker_service::processing;
using namespace worker_service::events;

using ::testing::_;
using ::testing::AllOf;
using ::testing::Field;
using ::testing::Return;

namespace {
constexpr auto VALID_ORDER_ID = "11111111-1111-1111-1111-111111111111";
constexpr auto VALID_USER_ID = "22222222-2222-2222-2222-222222222222";
constexpr auto VALID_PRODUCT_ID = "33333333-3333-3333-3333-333333333333";
constexpr auto VALID_EVENT_ID = "44444444-4444-4444-4444-444444444444";

nlohmann::json validEventJson() {
    return {{"orderId", VALID_ORDER_ID},
            {"userId", VALID_USER_ID},
            {"items", nlohmann::json::array({{{"productId", VALID_PRODUCT_ID}, {"quantity", 2}}})}};
}

shared::messaging::MessageMetadata validMetadata() {
    return shared::messaging::MessageMetadata{.eventId = shared::models::OutboxEventId::create(VALID_EVENT_ID).value(),
                                              .eventType = "OrderCreated"};
}
}  // namespace

class OrderReservationHandlerTest : public ::testing::Test {
   protected:
    MockWorkerRepository _workerRepository;
    MockOrderProcessor _orderProcessor;
    OrderReservationHandler _handler{_workerRepository, _orderProcessor};
};

TEST_F(OrderReservationHandlerTest, MalformedJsonIsSkippedAndOffsetCommitted) {
    EXPECT_CALL(_orderProcessor, process(_)).Times(0);
    EXPECT_CALL(_workerRepository, recordReservationIfNew(_)).Times(0);

    const auto result = _handler.handle("not valid json {{{", validMetadata());

    EXPECT_TRUE(result);
}

TEST_F(OrderReservationHandlerTest, InvalidEventIsSkippedAndOffsetCommitted) {
    auto json = validEventJson();
    json.erase("userId");

    EXPECT_CALL(_orderProcessor, process(_)).Times(0);
    EXPECT_CALL(_workerRepository, recordReservationIfNew(_)).Times(0);

    const auto result = _handler.handle(json.dump(), validMetadata());

    EXPECT_TRUE(result);
}

TEST_F(OrderReservationHandlerTest, ProcessorFailureIsNotCommitted) {
    EXPECT_CALL(_orderProcessor, process(_)).WillOnce(Return(std::unexpected(ProcessingError::ReservationFailed)));
    EXPECT_CALL(_workerRepository, recordReservationIfNew(_)).Times(0);

    const auto result = _handler.handle(validEventJson().dump(), validMetadata());

    EXPECT_FALSE(result);
}

TEST_F(OrderReservationHandlerTest, RepositoryFailureIsNotCommitted) {
    EXPECT_CALL(_orderProcessor, process(_)).WillOnce(Return(std::expected<void, std::error_code>{}));
    EXPECT_CALL(_workerRepository, recordReservationIfNew(_))
        .WillOnce(Return(std::unexpected(shared::repository::RepositoryError::ConnectionFailure)));

    const auto result = _handler.handle(validEventJson().dump(), validMetadata());

    EXPECT_FALSE(result);
}

TEST_F(OrderReservationHandlerTest, DuplicateEventIsStillCommitted) {
    EXPECT_CALL(_orderProcessor, process(_)).WillOnce(Return(std::expected<void, std::error_code>{}));
    // false без ошибки = дубликат, ничего не записано повторно
    EXPECT_CALL(_workerRepository, recordReservationIfNew(_)).WillOnce(Return(false));

    const auto result = _handler.handle(validEventJson().dump(), validMetadata());

    EXPECT_TRUE(result);
}

TEST_F(OrderReservationHandlerTest, HappyPathIsCommitted) {
    EXPECT_CALL(_orderProcessor, process(_)).WillOnce(Return(std::expected<void, std::error_code>{}));
    EXPECT_CALL(_workerRepository, recordReservationIfNew(_)).WillOnce(Return(true));

    const auto result = _handler.handle(validEventJson().dump(), validMetadata());

    EXPECT_TRUE(result);
}

TEST_F(OrderReservationHandlerTest, ProcessorReceivesEventParsedFromPayload) {
    EXPECT_CALL(
        _orderProcessor,
        process(AllOf(Field(&OrderCreatedEvent::orderId, shared::models::OrderId::create(VALID_ORDER_ID).value()),
                      Field(&OrderCreatedEvent::userId, shared::models::UserId::create(VALID_USER_ID).value()))))
        .WillOnce(Return(std::expected<void, std::error_code>{}));

    EXPECT_CALL(_workerRepository, recordReservationIfNew(_)).WillOnce(Return(true));

    _handler.handle(validEventJson().dump(), validMetadata());
}

TEST_F(OrderReservationHandlerTest, RecordReservationReceivesEventIdFromMetadataNotFromPayload) {
    EXPECT_CALL(_orderProcessor, process(_)).WillOnce(Return(std::expected<void, std::error_code>{}));

    EXPECT_CALL(_workerRepository,
                recordReservationIfNew(
                    Field(&ReservationRecord::eventId, shared::models::OutboxEventId::create(VALID_EVENT_ID).value())))
        .WillOnce(Return(true));

    _handler.handle(validEventJson().dump(), validMetadata());
}

TEST_F(OrderReservationHandlerTest, RecordReservationUsesOrderIdAsAggregateId) {
    EXPECT_CALL(_orderProcessor, process(_)).WillOnce(Return(std::expected<void, std::error_code>{}));

    EXPECT_CALL(_workerRepository,
                recordReservationIfNew(
                    Field(&ReservationRecord::aggregateId, shared::models::OrderId::create(VALID_ORDER_ID).value())))
        .WillOnce(Return(true));

    _handler.handle(validEventJson().dump(), validMetadata());
}

TEST_F(OrderReservationHandlerTest, RecordReservationUsesOrderReservedEventType) {
    EXPECT_CALL(_orderProcessor, process(_)).WillOnce(Return(std::expected<void, std::error_code>{}));

    EXPECT_CALL(_workerRepository,
                recordReservationIfNew(Field(&ReservationRecord::eventType, std::string{"OrderReserved"})))
        .WillOnce(Return(true));

    _handler.handle(validEventJson().dump(), validMetadata());
}

TEST_F(OrderReservationHandlerTest, RecordReservationPayloadMatchesOrderReservedEventJson) {
    const auto expectedPayload =
        OrderReservedEventJsonMapper::toJson(
            OrderReservedEvent{.orderId = shared::models::OrderId::create(VALID_ORDER_ID).value()})
            .dump();

    EXPECT_CALL(_orderProcessor, process(_)).WillOnce(Return(std::expected<void, std::error_code>{}));

    EXPECT_CALL(_workerRepository, recordReservationIfNew(Field(&ReservationRecord::payload, expectedPayload)))
        .WillOnce(Return(true));

    _handler.handle(validEventJson().dump(), validMetadata());
}

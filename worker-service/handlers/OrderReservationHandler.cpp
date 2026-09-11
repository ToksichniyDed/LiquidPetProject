//
// Created by DED on 10.09.2026.
//

#include "OrderReservationHandler.h"

#include <OrderCreatedEvent.h>
#include <OrderReservedEvent.h>
#include <logging/Logger.h>
#include <mapper/OrderCreatedEventJsonMapper.h>
#include <mapper/OrderReservedEventJsonMapper.h>

namespace worker_service::handlers {
using namespace shared::logger;

OrderReservationHandler::OrderReservationHandler(repository::IWorkerRepository& workerRepository,
                                                 processing::IOrderProcessor& orderProcessor)
    : _workerRepository(workerRepository), _orderProcessor(orderProcessor) {}

bool OrderReservationHandler::handle(const std::string& payload) {
    auto eventResult = events::OrderCreatedEventJsonMapper::fromJson(nlohmann::json::parse(payload, nullptr, false));
    if (!eventResult.has_value()) {
        SPDLOG_LOGGER_ERROR(get("OrderReservationHandler"), "Failed to parse OrderCreatedEvent, skipping message");
        return true;
    }

    const auto& event = eventResult.value();

    if (auto processResult = _orderProcessor.process(event); !processResult.has_value()) {
        SPDLOG_LOGGER_ERROR(get("OrderReservationHandler"), "Reservation failed for orderId={}", event.orderId.value());
        return false;
    }

    const events::OrderReservedEvent reservedEvent{.orderId = event.orderId};
    const auto reservedPayload = events::OrderReservedEventJsonMapper::toJson(reservedEvent).dump();

    auto repositoryResult = _workerRepository.recordReservationIfNew(repository::ReservationRecord{
        .eventId = event.eventId,
        .aggregateId = event.orderId.value(),
        .eventType = "OrderReserved",
        .payload = reservedPayload,
    });

    if (!repositoryResult.has_value()) {
        SPDLOG_LOGGER_ERROR(get("OrderReservationHandler"), "Failed to record reservation for eventId={}",
                            event.eventId);
        return false;
    }

    if (!repositoryResult.value()) {
        SPDLOG_LOGGER_INFO(get("OrderReservationHandler"), "eventId={} already processed, skipping", event.eventId);
    }

    return true;
}
}  // namespace worker_service::handlers

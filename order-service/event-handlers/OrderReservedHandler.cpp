//
// Created by DED on 19.09.2026.
//

#include "OrderReservedHandler.h"

#include <logging/Logger.h>

#include <nlohmann/json.hpp>

#include "OrderReservedEventJsonMapper.h"

namespace order_service::event_handlers {
using namespace shared::logger;
using order_system::models::Order;
using order_system::models::OrderStatusMapper;
using order_system::repository::RepositoryError;

OrderReservedHandler::OrderReservedHandler(order_system::repository::IOrderRepository& orderRepository,
                                           order_system::repository::IOrderStatusRepository& orderStatusRepository)
    : _orderRepository(orderRepository), _orderStatusRepository(orderStatusRepository) {}

bool OrderReservedHandler::handle(const std::string& payload, const shared::messaging::MessageMetadata& metadata) {
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(payload);
    } catch (const nlohmann::json::parse_error&) {
        SPDLOG_LOGGER_ERROR(get("OrderReservedHandler"), "Malformed JSON, skipping message");
        return true;
    }

    const auto event = OrderReservedEventJsonMapper::fromJson(json);
    if (!event.has_value()) {
        SPDLOG_LOGGER_ERROR(get("OrderReservedHandler"), "Failed to parse OrderReserved event: {}",
                            event.error().message());
        return true;
    }

    const auto& orderId = event->orderId;

    auto order = _orderRepository.findById(orderId);
    if (!order.has_value()) {
        if (order.error() == RepositoryError::NotFound) {
            SPDLOG_LOGGER_ERROR(get("OrderReservedHandler"), "Order {} from eventId={} not found, skipping",
                                orderId.value(), metadata.eventId.value());
            return true;
        }

        SPDLOG_LOGGER_WARN(get("OrderReservedHandler"), "Failed to load order {}: {}, will retry", orderId.value(),
                           order.error().message());
        return false;
    }

    if (order->status() == Order::OrderStatus::Reserved) {
        SPDLOG_LOGGER_INFO(get("OrderReservedHandler"), "Order {} already reserved, skipping duplicate eventId={}",
                           orderId.value(), metadata.eventId.value());
        return true;
    }

    const auto previousStatus = order->status();
    if (const auto transition = order->markReserved(); !transition.has_value()) {
        // Бизнес-конфликт : повторять бессмысленно
        SPDLOG_LOGGER_WARN(get("OrderReservedHandler"), "Order {} cannot be reserved from status {}: {}",
                           orderId.value(), OrderStatusMapper::toString(previousStatus),
                           transition.error().message());
        return true;
    }

    const auto changed = _orderStatusRepository.changeStatus(orderId, previousStatus, order->status());
    if (!changed.has_value()) {
        SPDLOG_LOGGER_WARN(get("OrderReservedHandler"), "Failed to change status of order {}: {}, will retry",
                           orderId.value(), changed.error().message());
        return false;
    }

    if (!changed.value()) {
        // Статус поменял кто-то параллельно: перечитаем заказ на повторе и решим заново
        SPDLOG_LOGGER_INFO(get("OrderReservedHandler"), "Order {} changed concurrently, will retry", orderId.value());
        return false;
    }

    SPDLOG_LOGGER_INFO(get("OrderReservedHandler"), "Order {} reserved (eventId={})", orderId.value(),
                       metadata.eventId.value());
    return true;
}

}

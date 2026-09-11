//
// Created by DED on 10.09.2026.
//

#include "OrderReservationProcessor.h"

#include <thread>

#include <logging/Logger.h>

namespace worker_service::processing {
using namespace shared::logger;

OrderReservationProcessor::OrderReservationProcessor(std::chrono::milliseconds simulatedDelay)
    : _simulatedDelay(simulatedDelay) {}

std::expected<void, std::error_code> OrderReservationProcessor::process(const events::OrderCreatedEvent& event) {
    SPDLOG_LOGGER_INFO(get("OrderReservationProcessor"), "Reserving items for orderId={}", event.orderId.value());

    // Эмуляция резервирования товара
    std::this_thread::sleep_for(_simulatedDelay);

    SPDLOG_LOGGER_INFO(get("OrderReservationProcessor"), "Reservation completed for orderId={}",
                        event.orderId.value());

    return {};
}

}

//
// Created by DED on 19.09.2026.
//

#ifndef LIQUIDPETPROJECT_ORDERRESERVEDEVENTJSONMAPPER_H
#define LIQUIDPETPROJECT_ORDERRESERVEDEVENTJSONMAPPER_H

#include <json/Json.h>

#include "OrderReservedEvent.h"
#include "keys/OrderReservedEventJsonKeys.h"

namespace order_service::event_handlers {

    class OrderReservedEventJsonMapper {
    public:
        static std::expected<OrderReservedEvent, std::error_code> fromJson(const nlohmann::json& json) {
            return shared::json::JsonHelper::getValue<std::string>(json, keys::ORDER_ID)
                .and_then([](std::string orderId) { return shared::models::OrderId::create(std::move(orderId)); })
                .transform([](shared::models::OrderId orderId) {
                    return OrderReservedEvent{.orderId = std::move(orderId)};
                });
        }
    };

}

#endif //LIQUIDPETPROJECT_ORDERRESERVEDEVENTJSONMAPPER_H

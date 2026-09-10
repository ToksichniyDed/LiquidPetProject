//
// Created by DED on 10.09.2026.
//

#ifndef LIQUIDPETPROJECT_ORDERCREATEDEVENT_H
#define LIQUIDPETPROJECT_ORDERCREATEDEVENT_H

#include <cstdint>
#include <expected>
#include <string>

#include <models/OrderIds.h>

namespace worker_service::events {

enum class OrderCreatedEventParseError : std::uint8_t { MalformedJson, MissingField, InvalidId };

struct OrderCreatedItem
{
    shared::models::ProductId productId;
    int quantity;
};

struct OrderCreatedEvent
{
    std::int64_t eventId;
    shared::models::OrderId orderId;
    shared::models::UserId userId;
    std::vector<OrderCreatedItem> items;
};
}

#endif //LIQUIDPETPROJECT_ORDERCREATEDEVENT_H

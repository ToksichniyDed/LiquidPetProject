//
// Created by DED on 10.09.2026.
//

#ifndef LIQUIDPETPROJECT_ORDERCREATEDEVENTJSONMAPPER_H
#define LIQUIDPETPROJECT_ORDERCREATEDEVENTJSONMAPPER_H

#include <json/Json.h>

#include <cstdint>
#include <expected>
#include <string>

#include "OrderCreatedEvent.h"
#include "keys/OrderCreatedEventJsonKeys.h"

namespace worker_service::events {

class OrderCreatedEventJsonMapper {
   public:
    static std::expected<OrderCreatedEvent, std::error_code> fromJson(const nlohmann::json& json) {
        auto orderIdString = shared::json::JsonHelper::getValue<std::string>(json, keys::ORDER_ID);
        if (!orderIdString.has_value())
            return std::unexpected(orderIdString.error());

        auto orderId = shared::models::OrderId::create(std::move(orderIdString.value()));
        if (!orderId.has_value())
            return std::unexpected(orderId.error());

        auto userIdString = shared::json::JsonHelper::getValue<std::string>(json, keys::USER_ID);
        if (!userIdString.has_value())
            return std::unexpected(userIdString.error());

        auto userId = shared::models::UserId::create(std::move(userIdString.value()));
        if (!userId.has_value())
            return std::unexpected(userId.error());

        auto items = parseItems(json);
        if (!items.has_value())
            return std::unexpected(items.error());

        return OrderCreatedEvent{
            .orderId = std::move(orderId.value()),
            .userId = std::move(userId.value()),
            .items = std::move(items.value())
        };
    }
    static nlohmann::json toJson(const OrderCreatedEvent& event) {
        nlohmann::json json;

        json[keys::ORDER_ID] = event.orderId.value();
        json[keys::USER_ID] = event.userId.value();

        nlohmann::json itemsJson = nlohmann::json::array();

        for (const auto& item : event.items) {
            nlohmann::json itemJson;
            itemJson[keys::PRODUCT_ID] = item.productId.value();
            itemJson[keys::QUANTITY] = item.quantity;
            itemsJson.push_back(std::move(itemJson));
        }

        json[keys::ITEMS] = std::move(itemsJson);
        return json;
    }

   private:
    static std::expected<std::vector<OrderCreatedItem>, std::error_code> parseItems(const nlohmann::json& json) {
        auto itemsJson = shared::json::JsonHelper::getValue<nlohmann::json>(json, keys::ITEMS);
        if (!itemsJson.has_value())
            return std::unexpected(itemsJson.error());

        if (!itemsJson.value().is_array())
            return std::unexpected(std::make_error_code(std::errc::invalid_argument));

        std::vector<OrderCreatedItem> items;
        items.reserve(itemsJson.value().size());
        for (const auto& itemJson : itemsJson.value()) {
            auto item = parseItem(itemJson);
            if (!item.has_value()) return std::unexpected(item.error());
            items.push_back(std::move(item.value()));
        }

        return items;
    }
    static std::expected<OrderCreatedItem, std::error_code> parseItem(const nlohmann::json& json) {
        auto productIdString = shared::json::JsonHelper::getValue<std::string>(json, keys::PRODUCT_ID);
        if (!productIdString.has_value())
            return std::unexpected(productIdString.error());

        auto productId = shared::models::ProductId::create(std::move(productIdString.value()));
        if (!productId.has_value())
            return std::unexpected(productId.error());

        auto quantity = shared::json::JsonHelper::getValue<int>(json, keys::QUANTITY);
        if (!quantity.has_value())
            return std::unexpected(quantity.error());

        return OrderCreatedItem{
            .productId = std::move(productId.value()),
            .quantity = quantity.value(),
        };
    }
};

}  // namespace worker_service::events

#endif  // LIQUIDPETPROJECT_ORDERCREATEDEVENTJSONMAPPER_H

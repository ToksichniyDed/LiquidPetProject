//
// Created by DED on 28.08.2026.
//

#ifndef LIQUIDPETPROJECT_ORDERJSONPARSER_H
#define LIQUIDPETPROJECT_ORDERJSONPARSER_H

#include <Order.h>
#include <keys/OrderJsonKeys.h>
#include <mapper/MoneyJsonMapper.h>
#include <json/Json.h>

#include <utility>

namespace order_system::models2json_mapper {
    using namespace order_system::models;
    using namespace Json;
    using namespace models2json_mapper::keys;

    class OrderItemJsonMapper {
    public:
        static std::expected<OrderItem, std::error_code> fromJson(const nlohmann::json& json) {
            return JsonHelper::getValue<std::string>(json, ORDER_ITEM_PRODUCT_ID)
                   .and_then([](std::string productId) {
                       return ProductId::create(std::move(productId));
                   })
                   .and_then([&json](ProductId productId) {
                       return JsonHelper::getValue<nlohmann::json>(json, ORDER_ITEM_PRICE_AT_ORDER_TIME)
                               .and_then([productId = std::move(productId)](const nlohmann::json& priceJson) mutable {
                                   return MoneyJsonMapper::fromJson(priceJson)
                                           .transform([productId = std::move(productId)](Money price) mutable {
                                               return std::pair{std::move(productId), std::move(price)};
                                           });
                               });
                   })
                   .and_then([&json](std::pair<ProductId, Money> productIdAndPrice) {
                       return JsonHelper::getValue<int>(json, ORDER_ITEM_QUANTITY)
                               .and_then([&productIdAndPrice](const int quantity) {
                                   auto& [productId, price] = productIdAndPrice;
                                   return OrderItem::create(std::move(productId), quantity, std::move(price));
                               });
                   });
        }

        static nlohmann::json toJson(const OrderItem& item) {
            nlohmann::json json;
            json[ORDER_ITEM_PRODUCT_ID] = item.productId().value();
            json[ORDER_ITEM_QUANTITY] = item.quantity();
            json[ORDER_ITEM_PRICE_AT_ORDER_TIME] = MoneyJsonMapper::toJson(item.priceAtOrderTime());
            return json;
        }
    };

    class OrderJsonMapper {
    public:
        static std::expected<Order, std::error_code> fromJson(const nlohmann::json& json) {
            return JsonHelper::getValue<std::string>(json, ORDER_USER_ID)
                   .and_then([](std::string userId) {
                       return UserId::create(std::move(userId));
                   })
                   .and_then([&json](UserId userId) {
                       return parseItems(json).and_then([&userId](std::vector<OrderItem> items) {
                           return Order::create(std::move(userId), std::move(items));
                       });
                   });
        }

        static nlohmann::json toJson(const Order& order) {
            nlohmann::json json;
            json[ORDER_USER_ID] = order.userId().value();

            if (order.orderId().has_value())
                json[ORDER_ID] = order.orderId()->value();

            nlohmann::json itemsJson = nlohmann::json::array();
            for (const auto& item : order.items())
                itemsJson.push_back(OrderItemJsonMapper::toJson(item));
            json[ORDER_ITEMS] = std::move(itemsJson);

            json[ORDER_STATUS] = std::to_underlying(order.status());

            return json;
        }

    private:
        static std::expected<std::vector<OrderItem>, std::error_code> parseItems(const nlohmann::json& json) {
            return JsonHelper::getValue<nlohmann::json>(json, keys::ORDER_ITEMS)
                    .and_then([](
                    const nlohmann::json& itemsJson) -> std::expected<std::vector<OrderItem>, std::error_code> {
                            if (!itemsJson.is_array())
                                return std::unexpected(OrderError::InvalidItems);

                            std::vector<OrderItem> items;
                            items.reserve(itemsJson.size());

                            for (const auto& itemJson : itemsJson) {
                                auto item = OrderItemJsonMapper::fromJson(itemJson);
                                if (!item.has_value())
                                    return std::unexpected(item.error());

                                items.push_back(std::move(item.value()));
                            }

                            return items;
                        });
        }
    };
}

#endif //LIQUIDPETPROJECT_ORDERJSONPARSER_H

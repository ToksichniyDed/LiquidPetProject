//
// Created by DED on 30.08.2026.
//

#ifndef LIQUIDPETPROJECT_ORDERROWMAPPER_H
#define LIQUIDPETPROJECT_ORDERROWMAPPER_H

#include <expected>
#include <pqxx/pqxx>

#include <Order.h>
#include <columns/OrderRowColumns.h>

namespace order_system::repository {

    using namespace order_system::models;
    using namespace order_system::repository;

    class OrderItemRowMapper {
    public:
        static std::expected<OrderItem, std::error_code> fromRow(const pqxx::row_ref& itemRow) {
            return ProductId::create(itemRow[columns::ORDER_ITEM_PRODUCT_ID].as<std::string>())
                   .and_then([&itemRow](const ProductId& productId) {
                       return Currency::create(
                           itemRow[columns::ORDER_ITEM_PRICE_CURRENCY_CODE].as<std::string>(),
                           static_cast<std::int8_t>(
                               itemRow[columns::ORDER_ITEM_PRICE_CURRENCY_MINOR_DIGITS]
                               .as<std::int16_t>()
                           )
                       ).transform([productId = std::move(productId)](Currency currency) mutable {
                           return std::pair{std::move(productId), std::move(currency)};
                       });
                   })
                   .and_then([&itemRow](std::pair<ProductId, Currency> productIdAndCurrency) {
                       auto& [productId, currency] = productIdAndCurrency;
                       return Money::create(
                           itemRow[columns::ORDER_ITEM_PRICE_MINOR_UNITS].as<std::int64_t>(),
                           std::move(currency)
                       ).transform([productId = std::move(productId)](Money price) mutable {
                           return std::pair{std::move(productId), std::move(price)};
                       });
                   })
                   .and_then([&itemRow](std::pair<ProductId, Money> productIdAndPrice) {
                       auto& [productId, price] = productIdAndPrice;
                       return OrderItem::create(
                           std::move(productId),
                           itemRow[columns::ORDER_ITEM_QUANTITY].as<int>(),
                           std::move(price)
                       );
                   });
        }
    };

    class OrderRowMapper {
    public:
        static std::expected<Order, std::error_code> fromRows(const pqxx::row& orderRow, const pqxx::result& itemRows) {
            return UserId::create(orderRow[columns::ORDER_USER_ID].as<std::string>())
                   .and_then([&](const UserId& userId) {
                       return parseItems(itemRows)
                               .and_then([&userId](std::vector<OrderItem> items) {
                                   return Order::create(std::move(userId), std::move(items));
                               });
                   })
                   .and_then([&orderRow](Order order) {
                       return OrderId::create(orderRow[columns::ORDER_ID].as<std::string>())
                               .transform([order = std::move(order)](OrderId orderId) mutable {
                                   order.assignId(std::move(orderId));
                                   return std::move(order);
                               });
                   });
        }

    private:
        static std::expected<std::vector<OrderItem>, std::error_code> parseItems(const pqxx::result& itemRows) {
            std::vector<OrderItem> items;
            items.reserve(itemRows.size());

            for (const auto& itemRow : itemRows) {
                auto item = OrderItemRowMapper::fromRow(itemRow);
                if (!item.has_value())
                    return std::unexpected(item.error());

                items.push_back(std::move(item.value()));
            }

            return items;
        }
    };

}

#endif //LIQUIDPETPROJECT_ORDERROWMAPPER_H

//
// Created by DED on 15.08.2026.
//

#ifndef LIQUIDPETPROJECT_ORDER_H
#define LIQUIDPETPROJECT_ORDER_H

#include <chrono>
#include <optional>
#include <regex>
#include <string>
#include <vector>

#include "StrongID.h"
#include "Money.h"

namespace order_system::models {
    struct OrderIdTag {
    };

    struct UserIdTag {
    };

    struct ProductIdTag {
    };

    namespace detail {
        inline bool isValidUuid(const std::string& value) {
            static const std::regex pattern(
                R"(^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$)"
            );
            return std::regex_match(value, pattern);
        }
    }

    template <>
    struct IdTraits<OrderIdTag> {
        static bool isValid(const std::string& v) { return detail::isValidUuid(v); }
    };

    template <>
    struct IdTraits<UserIdTag> {
        static bool isValid(const std::string& v) { return detail::isValidUuid(v); }
    };

    template <>
    struct IdTraits<ProductIdTag> {
        static bool isValid(const std::string& v) { return detail::isValidUuid(v); }
    };

    using OrderId = StrongID<OrderIdTag>;
    using UserId = StrongID<UserIdTag>;
    using ProductId = StrongID<ProductIdTag>;

    class OrderItem {
    public:
        enum class Error {
            InvalidQuantity,
            PriceOverflow
        };

    public:
        static std::expected<OrderItem, Error> create(ProductId productId, int quantity, Money priceAtOrderTime) {
            if (quantity <= 0)
                return std::unexpected(Error::InvalidQuantity);

            return OrderItem{std::move(productId), quantity, std::move(priceAtOrderTime)};
        }

        const ProductId& productId() const { return _productId; }
        int quantity() const { return _quantity; }
        const Money& priceAtOrderTime() const { return _priceAtOrderTime; }

        std::expected<Money, Error> lineTotal() const {
            auto totalPrice = _priceAtOrderTime * _quantity;

            if (!totalPrice.has_value())
                return std::unexpected(Error::PriceOverflow);

            return totalPrice.value();
        }

    private:
        OrderItem(ProductId productId, int quantity, Money priceAtOrderTime) : _productId(std::move(productId)),
                                                                               _quantity(quantity),
                                                                               _priceAtOrderTime(
                                                                                   std::move(priceAtOrderTime)) {
        }

    private:
        ProductId _productId;
        int _quantity;
        Money _priceAtOrderTime;
    };

    class Order {
    public:
        enum class Error {
            EmptyItems,
            Unknown
        };

        enum class OrderStatus {
            Created,
            Reserved,
            Shipped,
            Cancelled
        };

    public:
        static std::expected<Order, Error> create(UserId userId, std::vector<OrderItem> items) {
            if (items.empty())
                return std::unexpected(Error::EmptyItems);

            return Order{std::move(userId), std::move(items)};
        }

        const UserId& userId() const { return _userId; }
        const std::optional<OrderId>& orderId() const { return _orderId; }
        const std::vector<OrderItem>& items() const { return _items; }
        OrderStatus status() const { return _status; }

        std::expected<Money, Error> totalAmount() const {
            auto currency = _items.front().priceAtOrderTime().currency();

            auto total = Money::create(0, currency);
            if (!total.has_value())
                return std::unexpected(Error::Unknown);

            Money accumulated = total.value();

            for (const auto& item : _items) {
                auto lineTotal = item.lineTotal();
                if (!lineTotal.has_value())
                    return std::unexpected(Error::Unknown);

                auto sum = accumulated + lineTotal.value();
                if (!sum.has_value())
                    return std::unexpected(Error::Unknown);

                accumulated = sum.value();
            }

            return accumulated;
        }

        void assignId(OrderId id) {
            _orderId = std::move(id);
        }

    private:
        explicit Order(UserId userId, std::vector<OrderItem> items) : _userId(std::move(userId)),
                                                                      _orderId(std::nullopt), _items(std::move(items)),
                                                                      _status(OrderStatus::Created) {
        }

    private:
        UserId _userId;
        std::optional<OrderId> _orderId;
        std::vector<OrderItem> _items;
        OrderStatus _status;
    };
};

#endif //LIQUIDPETPROJECT_ORDER_H

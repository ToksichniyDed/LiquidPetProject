//
// Created by DED on 15.08.2026.
//

#ifndef LIQUIDPETPROJECT_ORDER_H
#define LIQUIDPETPROJECT_ORDER_H

#include <chrono>
#include <optional>
#include <regex>
#include <string>
#include <system_error>
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

    enum class OrderItemError : std::uint8_t {
        InvalidQuantity = 1,
        PriceOverflow
    };

    class OrderItemErrorCategory : public std::error_category {
    public:
        const char* name() const noexcept override { return "orderItem"; }

        std::string message(int ev) const override {
            switch (static_cast<OrderItemError>(ev)) {
                case OrderItemError::InvalidQuantity:
                    return "invalid quantity";
                case OrderItemError::PriceOverflow:
                    return "price overflow";
                default:
                    return "unknown orderItem error";
            }
        }
    };

    inline const OrderItemErrorCategory& orderItemErrorCategory() {
        static OrderItemErrorCategory instance;
        return instance;
    }

    inline std::error_code make_error_code(OrderItemError e) {
        return {static_cast<int>(e), orderItemErrorCategory()};
    }

    enum class OrderError : std::uint8_t {
        EmptyItems = 1,
        InvalidItems,
        UnknownStatus
    };

    class OrderErrorCategory : public std::error_category {
    public:
        const char* name() const noexcept override { return "order"; }

        std::string message(int ev) const override {
            switch (static_cast<OrderError>(ev)) {
                case OrderError::EmptyItems:
                    return "empty items";
                case OrderError::InvalidItems:
                    return "invalid items";
                default:
                    return "unknown order error";
            }
        }
    };

    inline const OrderErrorCategory& orderErrorCategory() {
        static OrderErrorCategory instance;
        return instance;
    }

    inline std::error_code make_error_code(OrderError e) {
        return {static_cast<int>(e), orderErrorCategory()};
    }

}

namespace std {
    template <>
    struct is_error_code_enum<order_system::models::OrderItemError> : true_type {
    };

    template <>
    struct is_error_code_enum<order_system::models::OrderError> : true_type {
    };
}

namespace order_system::models {

    class OrderItem {
    public:
        static std::expected<OrderItem, std::error_code> create(ProductId productId, int quantity,
                                                                Money priceAtOrderTime) {
            if (quantity <= 0)
                return std::unexpected(OrderItemError::InvalidQuantity);

            return OrderItem{std::move(productId), quantity, std::move(priceAtOrderTime)};
        }

        const ProductId& productId() const { return _productId; }
        int quantity() const { return _quantity; }
        const Money& priceAtOrderTime() const { return _priceAtOrderTime; }

        std::expected<Money, std::error_code> lineTotal() const {
            auto totalPrice = _priceAtOrderTime * _quantity;

            if (!totalPrice.has_value())
                return std::unexpected(OrderItemError::PriceOverflow);

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
        enum class OrderStatus { Created, Reserved, Shipped, Cancelled };

    public:
        static std::expected<Order, std::error_code> create(UserId userId, std::vector<OrderItem> items) {
            if (items.empty())
                return std::unexpected(OrderError::EmptyItems);

            return Order{std::move(userId), std::move(items)};
        }

        const UserId& userId() const { return _userId; }
        const std::optional<OrderId>& orderId() const { return _orderId; }
        const std::vector<OrderItem>& items() const { return _items; }
        OrderStatus status() const { return _status; }

        std::expected<Money, std::error_code> totalAmount() const {
            auto currency = _items.front().priceAtOrderTime().currency();

            std::expected<Money, std::error_code> total = Money::create(0, currency);

            for (const auto& item : _items) {
                if (!total.has_value())
                    return total;

                auto lineTotal = item.lineTotal();
                if (!lineTotal.has_value())
                    return std::unexpected(lineTotal.error());

                total = total.value() + lineTotal.value();
            }

            return total;
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

    class OrderStatusMapper {
    public:
        static std::string toString(Order::OrderStatus status) {
            switch (status) {
                case Order::OrderStatus::Created:
                    return "Created";
                case Order::OrderStatus::Reserved:
                    return "Reserved";
                case Order::OrderStatus::Shipped:
                    return "Shipped";
                case Order::OrderStatus::Cancelled:
                    return "Cancelled";
            }
            std::unreachable();
        }

        static std::expected<Order::OrderStatus, std::error_code> fromString(const std::string& value) {
            if (value == "Created")
                return Order::OrderStatus::Created;
            if (value == "Reserved")
                return Order::OrderStatus::Reserved;
            if (value == "Shipped")
                return Order::OrderStatus::Shipped;
            if (value == "Cancelled")
                return Order::OrderStatus::Cancelled;
            return std::unexpected(OrderError::UnknownStatus);
        }
    };
}

#endif //LIQUIDPETPROJECT_ORDER_H

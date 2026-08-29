//
// Created by DED on 29.08.2026.
//

#include <gtest/gtest.h>

#include <limits>
#include <vector>

#include "Order.h"

using namespace order_system::models;

namespace {

    Currency createUsd() {
        return Currency::create("USD", 2).value();
    }

    Currency createEur() {
        return Currency::create("EUR", 2).value();
    }

    UserId createUserId() {
        return UserId::create("550e8400-e29b-41d4-a716-446655440000").value();
    }

    ProductId createProductId() {
        return ProductId::create("550e8400-e29b-41d4-a716-446655440001").value();
    }

    OrderId createOrderId() {
        return OrderId::create("550e8400-e29b-41d4-a716-446655440002").value();
    }

    Money createMoney(int64_t minorUnits, const Currency& currency) {
        return Money::create(minorUnits, currency).value();
    }

    OrderItem createOrderItem(
        int quantity,
        int64_t price,
        const Currency& currency = createUsd()
    ) {
        return OrderItem::create(
            createProductId(),
            quantity,
            createMoney(price, currency)
        ).value();
    }

    Order createOrder(std::vector<OrderItem> items) {
        return Order::create(
            createUserId(),
            std::move(items)
        ).value();
    }

    struct OrderCreateTestCase {
        std::string testName;
        std::vector<OrderItem> items;
        bool expectSuccess;
    };

    struct OrderTotalAmountTestCase {
        std::string testName;
        std::vector<OrderItem> items;
        bool expectSuccess;
        int64_t expectedAmount;
        std::string expectedCurrency;
    };

} // namespace


class OrderCreateTest
        : public ::testing::TestWithParam<OrderCreateTestCase> {
};

TEST_P(OrderCreateTest, Create) {
    const auto& testCase = GetParam();

    const auto result = Order::create(
        createUserId(),
        testCase.items
    );

    if (testCase.expectSuccess) {
        ASSERT_TRUE(result.has_value());

        EXPECT_EQ(result->userId(), createUserId());
        EXPECT_FALSE(result->orderId().has_value());
        EXPECT_EQ(result->status(), Order::OrderStatus::Created);
        EXPECT_EQ(result->items().size(), testCase.items.size());

        return;
    }

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), OrderError::EmptyItems);
}

INSTANTIATE_TEST_SUITE_P(
    OrderTests,
    OrderCreateTest,
    ::testing::Values(
        OrderCreateTestCase{
        .testName = "ValidOrder",
        .items = {
        createOrderItem(2, 1500)
        },
        .expectSuccess = true
        },
        OrderCreateTestCase{
        .testName = "ValidOrderWithMultipleItems",
        .items = {
        createOrderItem(2, 1500),
        createOrderItem(1, 2500),
        createOrderItem(3, 1000)
        },
        .expectSuccess = true
        },
        OrderCreateTestCase{
        .testName = "EmptyItems",
        .items = {},
        .expectSuccess = false
        }
    ),
    [](const ::testing::TestParamInfo<OrderCreateTestCase>& info) {
    return info.param.testName;
    }
);


class OrderTotalAmountTest
        : public ::testing::TestWithParam<OrderTotalAmountTestCase> {
};

TEST_P(OrderTotalAmountTest, TotalAmount) {
    const auto& testCase = GetParam();

    const auto order = Order::create(
        createUserId(),
        testCase.items
    );

    ASSERT_TRUE(order.has_value());

    const auto result = order->totalAmount();

    if (testCase.expectSuccess) {
        ASSERT_TRUE(result.has_value());

        EXPECT_EQ(
            result->minorUnits(),
            testCase.expectedAmount
        );

        EXPECT_EQ(
            result->currency().code(),
            testCase.expectedCurrency
        );

        return;
    }

    ASSERT_FALSE(result.has_value());
}

INSTANTIATE_TEST_SUITE_P(
    OrderTests,
    OrderTotalAmountTest,
    ::testing::Values(
        OrderTotalAmountTestCase{
        .testName = "SingleItem",
        .items = {
        createOrderItem(2, 1500)
        },
        .expectSuccess = true,
        .expectedAmount = 3000,
        .expectedCurrency = "USD"
        },
        OrderTotalAmountTestCase{
        .testName = "MultipleItems",
        .items = {
        createOrderItem(2, 1500),
        createOrderItem(1, 2500),
        createOrderItem(3, 1000)
        },
        .expectSuccess = true,
        .expectedAmount = 8500,
        .expectedCurrency = "USD"
        },
        OrderTotalAmountTestCase{
        .testName = "ZeroPrice",
        .items = {
        createOrderItem(2, 0)
        },
        .expectSuccess = true,
        .expectedAmount = 0,
        .expectedCurrency = "USD"
        },
        OrderTotalAmountTestCase{
        .testName = "PriceOverflow",
        .items = {
        createOrderItem(
            2,
            std::numeric_limits<int64_t>::max()
        )
        },
        .expectSuccess = false,
        .expectedAmount = 0,
        .expectedCurrency = ""
        }
    ),
    [](const ::testing::TestParamInfo<OrderTotalAmountTestCase>& info) {
    return info.param.testName;
    }
);


class OrderAssignIdTest : public ::testing::Test {
protected:
    Order _order = createOrder({
        createOrderItem(1, 1000)
    });
};

TEST_F(OrderAssignIdTest, AssignId) {
    const auto orderId = createOrderId();

    EXPECT_FALSE(_order.orderId().has_value());

    _order.assignId(orderId);

    ASSERT_TRUE(_order.orderId().has_value());
    EXPECT_EQ(_order.orderId().value(), orderId);
}

TEST_F(OrderAssignIdTest, ReassignId) {
    const auto firstOrderId = createOrderId();

    const auto secondOrderId =
            OrderId::create(
                "550e8400-e29b-41d4-a716-446655440003"
            ).value();

    _order.assignId(firstOrderId);
    ASSERT_TRUE(_order.orderId().has_value());
    EXPECT_EQ(_order.orderId().value(), firstOrderId);

    _order.assignId(secondOrderId);

    ASSERT_TRUE(_order.orderId().has_value());
    EXPECT_EQ(_order.orderId().value(), secondOrderId);
}


class OrderStatusTest : public ::testing::Test {
protected:
    Order _order = createOrder({
        createOrderItem(1, 1000)
    });
};

TEST_F(OrderStatusTest, NewOrderHasCreatedStatus) {
    EXPECT_EQ(
        _order.status(),
        Order::OrderStatus::Created
    );
}


class OrderItemsTest : public ::testing::Test {
protected:
    std::vector<OrderItem> _items = {
        createOrderItem(2, 1500),
        createOrderItem(1, 2500)
    };

    Order _order = createOrder(_items);
};

TEST_F(OrderItemsTest, ReturnsAllItems) {
    const auto& items = _order.items();

    ASSERT_EQ(items.size(), 2);

    EXPECT_EQ(items[0].quantity(), 2);
    EXPECT_EQ(items[0].priceAtOrderTime().minorUnits(), 1500);

    EXPECT_EQ(items[1].quantity(), 1);
    EXPECT_EQ(items[1].priceAtOrderTime().minorUnits(), 2500);
}

TEST_F(OrderItemsTest, ReturnsItemsInOriginalOrder) {
    const auto& items = _order.items();

    ASSERT_EQ(items.size(), _items.size());

    for (std::size_t i = 0; i < items.size(); ++i) {
        EXPECT_EQ(
            items[i].quantity(),
            _items[i].quantity()
        );

        EXPECT_EQ(
            items[i].priceAtOrderTime(),
            _items[i].priceAtOrderTime()
        );
    }
}


class OrderUserIdTest : public ::testing::Test {
protected:
    UserId _userId = createUserId();

    Order _order = Order::create(
        _userId,
        {createOrderItem(1, 1000)}
    ).value();
};

TEST_F(OrderUserIdTest, ReturnsUserId) {
    EXPECT_EQ(_order.userId(), _userId);
}


class OrderTotalAmountCurrencyTest : public ::testing::Test {
};

TEST_F(OrderTotalAmountCurrencyTest, UsesCurrencyFromFirstItem) {
    const auto usd = createUsd();

    const auto order = Order::create(
        createUserId(),
        {
            createOrderItem(1, 1000, usd),
            createOrderItem(2, 500, usd)
        }
    );

    ASSERT_TRUE(order.has_value());

    const auto result = order->totalAmount();

    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(result->currency().code(), "USD");
    EXPECT_EQ(result->minorUnits(), 2000);
}

TEST_F(OrderTotalAmountCurrencyTest, DifferentCurrenciesReturnsError) {
    const auto order = Order::create(
        createUserId(),
        {
            createOrderItem(1, 1000, createUsd()),
            createOrderItem(1, 1000, createEur())
        }
    );

    ASSERT_TRUE(order.has_value());

    const auto result = order->totalAmount();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), MoneyError::DifferentCurrencies);
}


class OrderTotalAmountOverflowTest : public ::testing::Test {
};

TEST_F(OrderTotalAmountOverflowTest, LineTotalOverflow) {
    const auto order = Order::create(
        createUserId(),
        {
            createOrderItem(
                2,
                std::numeric_limits<int64_t>::max()
            )
        }
    );

    ASSERT_TRUE(order.has_value());

    const auto result = order->totalAmount();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), OrderItemError::PriceOverflow);
}

TEST_F(OrderTotalAmountOverflowTest, TotalSumOverflow) {
    const auto order = Order::create(
        createUserId(),
        {
            createOrderItem(
                1,
                std::numeric_limits<int64_t>::max()
            ),
            createOrderItem(
                1,
                1
            )
        }
    );

    ASSERT_TRUE(order.has_value());

    const auto result = order->totalAmount();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), MoneyError::Overflow);
}

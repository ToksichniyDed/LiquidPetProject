//
// Created by DED on 15.08.2026.
//

#include <gtest/gtest.h>

#include "Currency.h"

using namespace order_system::models;

TEST(CurrencyTest, CreateSucceedsWithValidAmount) {
    auto currency = Currency::create("RUB", 2);

    ASSERT_TRUE(currency.has_value());
    EXPECT_EQ(currency->code(), "RUB");
    EXPECT_EQ(currency->minorDigits(), 2);
}

TEST(CurrencyTest, CreateFailsWithInvalidAmount) {
    auto currency = Currency::create("RUB", -1);

    ASSERT_FALSE(currency.has_value());
    EXPECT_EQ(currency.error(), CurrencyError::NegativeAmount);
}

TEST(CurrencyTest, CreateFailsWithEmptyCurrency) {
    auto currency = Currency::create("", 2);

    ASSERT_FALSE(currency.has_value());
    EXPECT_EQ(currency.error(), CurrencyError::EmptyCurrency);
}

TEST(CurrencyTest, CreateSucceedsWithZeroAmount) {
    auto currency = Currency::create("RUB", 0);

    ASSERT_TRUE(currency.has_value());
    EXPECT_EQ(currency->code(), "RUB");
    EXPECT_EQ(currency->minorDigits(), 0);
}

TEST(CurrencyTest, EqualOperator) {
    auto RUB2 = Currency::create("RUB", 2);
    auto RUB4 = Currency::create("RUB", 4);
    auto USD4 = Currency::create("USD", 4);

    ASSERT_TRUE(RUB2.has_value());
    ASSERT_TRUE(RUB4.has_value());
    ASSERT_TRUE(USD4.has_value());
    EXPECT_TRUE(RUB2.value() == RUB4.value());
    EXPECT_FALSE(RUB2.value() == USD4.value());
}



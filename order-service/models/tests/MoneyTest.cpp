//
// Created by DED on 15.08.2026.
//

#include <gtest/gtest.h>

#include "Money.h"
#include "Currency.h"

using namespace order_system::models;

TEST(MoneyTest, CreateSucceedsWithPositiveAmount) {
    auto currency = Currency::create("RUB", 2);

    ASSERT_TRUE(currency.has_value());

    auto money = Money::create(10, currency.value());
    ASSERT_TRUE(money.has_value());

    EXPECT_EQ(money.value().minorUnits(), 10);
    EXPECT_EQ(money.value().currency().code(), currency.value().code());
    EXPECT_EQ(money.value().currency().minorDigits(), currency.value().minorDigits());
}

TEST(MoneyTest, CreateSucceedsWithZeroAmount) {
    auto currency = Currency::create("RUB", 2);

    ASSERT_TRUE(currency.has_value());

    auto money = Money::create(0, currency.value());
    ASSERT_TRUE(money.has_value());

    EXPECT_EQ(money.value().minorUnits(), 0);
    EXPECT_EQ(money.value().currency().code(), currency.value().code());
    EXPECT_EQ(money.value().currency().minorDigits(), currency.value().minorDigits());
}

TEST(MoneyTest, CreateFailsWithNegativeAmount) {
    auto currency = Currency::create("RUB", 2);

    ASSERT_TRUE(currency.has_value());

    auto money = Money::create(-1, currency.value());
    ASSERT_FALSE(money.has_value());

    EXPECT_EQ(money.error(), Money::Error::NegativeAmount);
}

TEST(MoneyTest, EqualOperatorTrueForSameAmountAndCurrency) {
    auto currency = Currency::create("RUB", 2);

    ASSERT_TRUE(currency.has_value());

    auto money1 = Money::create(4, currency.value());
    auto money2 = Money::create(4, currency.value());

    ASSERT_TRUE(money1.has_value());
    ASSERT_TRUE(money2.has_value());

    EXPECT_TRUE(money1.value() == money2.value());
}

TEST(MoneyTest, EqualOperatorFalseForDifferentAmount) {
    auto currency = Currency::create("RUB", 2);

    ASSERT_TRUE(currency.has_value());

    auto money1 = Money::create(4, currency.value());
    auto money2 = Money::create(5, currency.value());

    ASSERT_TRUE(money1.has_value());
    ASSERT_TRUE(money2.has_value());

    EXPECT_FALSE(money1.value() == money2.value());
}

TEST(MoneyTest, EqualOperatorFalseForDifferentCurrency) {
    auto currency1 = Currency::create("RUB", 2);
    auto currency2 = Currency::create("USD", 2);

    ASSERT_TRUE(currency1.has_value());
    ASSERT_TRUE(currency2.has_value());

    auto money1 = Money::create(4, currency1.value());
    auto money2 = Money::create(4, currency2.value());

    ASSERT_TRUE(money1.has_value());
    ASSERT_TRUE(money2.has_value());

    EXPECT_FALSE(money1.value() == money2.value());
}

TEST(MoneyTest, AdditionSucceedsForSameCurrency) {
    auto currency = Currency::create("RUB", 2);

    ASSERT_TRUE(currency.has_value());

    auto money1 = Money::create(1, currency.value());
    auto money2 = Money::create(2, currency.value());

    ASSERT_TRUE(money1.has_value());
    ASSERT_TRUE(money2.has_value());

    auto money3 = money1.value() + money2.value();

    ASSERT_TRUE(money3.has_value());

    EXPECT_EQ(money3.value().minorUnits(), 3);
}

TEST(MoneyTest, AdditionFailsForDifferentCurrencies) {
    auto currency1 = Currency::create("RUB", 2);
    auto currency2 = Currency::create("USD", 2);

    ASSERT_TRUE(currency1.has_value());
    ASSERT_TRUE(currency2.has_value());

    auto money1 = Money::create(1, currency1.value());
    auto money2 = Money::create(2, currency2.value());

    ASSERT_TRUE(money1.has_value());
    ASSERT_TRUE(money2.has_value());

    auto money3 = money1.value() + money2.value();

    ASSERT_FALSE(money3.has_value());

    EXPECT_EQ(money3.error(), Money::Error::DifferentCurrencies);
}

TEST(MoneyTest, AdditionFailsOnOverflow) {
    auto currency = Currency::create("RUB", 2);
    ASSERT_TRUE(currency.has_value());

    auto money1 = Money::create(std::numeric_limits<int64_t>::max(), currency.value());
    auto money2 = Money::create(1, currency.value());

    ASSERT_TRUE(money1.has_value());
    ASSERT_TRUE(money2.has_value());

    auto result = money1.value() + money2.value();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), Money::Error::Overflow);
}

TEST(MoneyTest, AdditionSucceedsAtMaxBoundary) {
    auto currency = Currency::create("RUB", 2);
    ASSERT_TRUE(currency.has_value());

    auto money1 = Money::create(std::numeric_limits<int64_t>::max() - 1, currency.value());
    auto money2 = Money::create(1, currency.value());

    ASSERT_TRUE(money1.has_value());
    ASSERT_TRUE(money2.has_value());

    auto result = money1.value() + money2.value();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().minorUnits(), std::numeric_limits<int64_t>::max());
}

TEST(MoneyTest, SubtractionSucceedsForSameCurrency) {
    auto currency = Currency::create("RUB", 2);
    ASSERT_TRUE(currency.has_value());

    auto money1 = Money::create(10, currency.value());
    auto money2 = Money::create(3, currency.value());

    ASSERT_TRUE(money1.has_value());
    ASSERT_TRUE(money2.has_value());

    auto result = money1.value() - money2.value();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().minorUnits(), 7);
}

TEST(MoneyTest, SubtractionFailsForDifferentCurrencies) {
    auto currency1 = Currency::create("RUB", 2);
    auto currency2 = Currency::create("USD", 2);

    ASSERT_TRUE(currency1.has_value());
    ASSERT_TRUE(currency2.has_value());

    auto money1 = Money::create(10, currency1.value());
    auto money2 = Money::create(3, currency2.value());

    ASSERT_TRUE(money1.has_value());
    ASSERT_TRUE(money2.has_value());

    auto result = money1.value() - money2.value();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), Money::Error::DifferentCurrencies);
}

TEST(MoneyTest, SubtractionFailsOnUnderflow) {
    auto currency = Currency::create("RUB", 2);
    ASSERT_TRUE(currency.has_value());

    auto money1 = Money::create(3, currency.value());
    auto money2 = Money::create(10, currency.value());

    ASSERT_TRUE(money1.has_value());
    ASSERT_TRUE(money2.has_value());

    auto result = money1.value() - money2.value();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), Money::Error::Underflow);
}

TEST(MoneyTest, SubtractionSucceedsWhenResultIsZero) {
    auto currency = Currency::create("RUB", 2);
    ASSERT_TRUE(currency.has_value());

    auto money1 = Money::create(7, currency.value());
    auto money2 = Money::create(7, currency.value());

    ASSERT_TRUE(money1.has_value());
    ASSERT_TRUE(money2.has_value());

    auto result = money1.value() - money2.value();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().minorUnits(), 0);
}

TEST(MoneyTest, MultiplicationSucceedsWithPositiveQuantity) {
    auto currency = Currency::create("RUB", 2);
    ASSERT_TRUE(currency.has_value());

    auto money = Money::create(150, currency.value());
    ASSERT_TRUE(money.has_value());

    auto result = money.value() * 3;

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().minorUnits(), 450);
}

TEST(MoneyTest, MultiplicationByZeroReturnsZero) {
    auto currency = Currency::create("RUB", 2);
    ASSERT_TRUE(currency.has_value());

    auto money = Money::create(150, currency.value());
    ASSERT_TRUE(money.has_value());

    auto result = money.value() * 0;

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().minorUnits(), 0);
}

TEST(MoneyTest, MultiplicationFailsWithNegativeQuantity) {
    auto currency = Currency::create("RUB", 2);
    ASSERT_TRUE(currency.has_value());

    auto money = Money::create(150, currency.value());
    ASSERT_TRUE(money.has_value());

    auto result = money.value() * -3;

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), Money::Error::NegativeAmount);
}

TEST(MoneyTest, MultiplicationFailsOnOverflow) {
    auto currency = Currency::create("RUB", 2);
    ASSERT_TRUE(currency.has_value());

    auto money = Money::create(std::numeric_limits<int64_t>::max() / 2, currency.value());
    ASSERT_TRUE(money.has_value());

    auto result = money.value() * 3;

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), Money::Error::Overflow);
}

TEST(MoneyTest, MinorUnitsReturnsConstructedValue) {
    auto currency = Currency::create("RUB", 2);
    ASSERT_TRUE(currency.has_value());

    auto money = Money::create(42, currency.value());
    ASSERT_TRUE(money.has_value());

    EXPECT_EQ(money.value().minorUnits(), 42);
}

TEST(MoneyTest, CurrencyReturnsConstructedValue) {
    auto currency = Currency::create("RUB", 2);
    ASSERT_TRUE(currency.has_value());

    auto money = Money::create(42, currency.value());
    ASSERT_TRUE(money.has_value());

    EXPECT_EQ(money.value().currency().code(), "RUB");
    EXPECT_EQ(money.value().currency().minorDigits(), 2);
}

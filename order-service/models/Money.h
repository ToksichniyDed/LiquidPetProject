//
// Created by DED on 15.08.2026.
//

#ifndef LIQUIDPETPROJECT_MONEY_H
#define LIQUIDPETPROJECT_MONEY_H

#include <expected>
#include <limits>
#include <string>
#include <system_error>
#include <utility>

#include "Currency.h"

namespace order_system::models {

    enum class MoneyError {
        NegativeAmount = 1,
        DifferentCurrencies,
        Underflow,
        Overflow
    };

    class MoneyErrorCategory : public std::error_category {
    public:
        const char* name() const noexcept override { return "money"; }

        std::string message(int ev) const override {
            switch (static_cast<MoneyError>(ev)) {
                case MoneyError::NegativeAmount:
                    return "negative amount";
                case MoneyError::DifferentCurrencies:
                    return "different currencies";
                case MoneyError::Overflow:
                    return "overflow";
                case MoneyError::Underflow:
                    return "underflow";
                default:
                    return "unknown money error";
            }
        }
    };

    inline const MoneyErrorCategory& moneyErrorCategory() {
        static MoneyErrorCategory instance;
        return instance;
    }

    inline std::error_code make_error_code(MoneyError e) {
        return {static_cast<int>(e), moneyErrorCategory()};
    }

}

namespace std {
    template <>
    struct is_error_code_enum<order_system::models::MoneyError> : true_type {
    };
}

namespace order_system::models {

    class Money {
    public:
        static std::expected<Money, std::error_code> create(int64_t minorUnits, Currency currency) {
            if (minorUnits < 0)
                return std::unexpected(MoneyError::NegativeAmount);

            return Money{minorUnits, std::move(currency)};
        }

        bool operator==(const Money& other) const {
            return _currency == other._currency && _minorUnits == other._minorUnits;
        }

        std::expected<Money, std::error_code> operator+(const Money& other) const {
            if (_currency.code() != other._currency.code())
                return std::unexpected(MoneyError::DifferentCurrencies);

            if (_minorUnits > std::numeric_limits<int64_t>::max() - other.minorUnits())
                return std::unexpected(MoneyError::Overflow);

            return Money{_minorUnits + other._minorUnits, _currency};
        }

        std::expected<Money, std::error_code> operator-(const Money& other) const {
            if (_currency.code() != other._currency.code())
                return std::unexpected(MoneyError::DifferentCurrencies);

            if (_minorUnits < other._minorUnits)
                return std::unexpected(MoneyError::Underflow);

            return Money{_minorUnits - other._minorUnits, _currency};
        }

        std::expected<Money, std::error_code> operator*(const int quantity) const {
            if (quantity == 0)
                return Money{0, _currency};

            if (quantity < 0)
                return std::unexpected(MoneyError::NegativeAmount);

            if (_minorUnits > std::numeric_limits<int64_t>::max() / quantity)
                return std::unexpected(MoneyError::Overflow);

            return Money{_minorUnits * quantity, _currency};
        }

        int64_t minorUnits() const { return _minorUnits; }
        const Currency& currency() const { return _currency; }

    private:
        Money(int64_t minorUnits, Currency currency) : _minorUnits(minorUnits), _currency(std::move(currency)) {
        }

    private:
        std::int64_t _minorUnits;
        Currency _currency;
    };

}

#endif //LIQUIDPETPROJECT_MONEY_H

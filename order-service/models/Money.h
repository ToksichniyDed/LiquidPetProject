//
// Created by DED on 15.08.2026.
//

#ifndef LIQUIDPETPROJECT_MONEY_H
#define LIQUIDPETPROJECT_MONEY_H

#include <expected>
#include <limits>
#include <stdexcept>
#include <string>

#include "Currency.h"

namespace order_system::models {
    class Money {
    public:
        enum class MoneyError {
            DifferentCurrencies,
            Underflow,
            Overflow,
            NegativeAmount,
        };

    public:
        static std::expected<Money, MoneyError> create(int64_t minorUnits, Currency currency) {
            if (minorUnits < 0)
                return std::unexpected(MoneyError::NegativeAmount);

            return Money{minorUnits, currency};
        }

        bool operator ==(const Money& other) const {
            return _currency == other._currency && _minorUnits == other._minorUnits;
        }

        std::expected<Money, MoneyError> operator +(const Money& other) const {
            if (_currency.code() != other._currency.code())
                return std::unexpected(MoneyError::DifferentCurrencies);

            if (_minorUnits > std::numeric_limits<int64_t>::max() - other.minorUnits())
                return std::unexpected(MoneyError::Overflow);

            return Money{_minorUnits + other._minorUnits, _currency};
        }

        std::expected<Money, MoneyError> operator -(const Money& other) const {
            if (_currency.code() != other._currency.code())
                return std::unexpected(MoneyError::DifferentCurrencies);

            if (_minorUnits < other._minorUnits)
                return std::unexpected(MoneyError::Underflow);

            return Money{_minorUnits - other._minorUnits, _currency};
        }

        std::expected<Money, MoneyError> operator *(int quantity) const {
            if (quantity == 0)
                return Money{0, _currency};

            if (quantity < 0)
                return std::unexpected(MoneyError::NegativeAmount);

            if (_minorUnits > std::numeric_limits<int64_t>::max() / quantity)
                return std::unexpected(MoneyError::Overflow);

            return Money{_minorUnits * quantity, _currency};
        }

        int64_t minorUnits() const {
            return _minorUnits;
        }

        const Currency& currency() const {
            return _currency;
        }

    private:
        Money(int64_t minorUnits, Currency currency) : _minorUnits(minorUnits), _currency(std::move(currency)) {
        };

    private:
        std::int64_t _minorUnits;
        Currency _currency;
    };
}


#endif //LIQUIDPETPROJECT_MONEY_H

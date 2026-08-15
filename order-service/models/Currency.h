//
// Created by DED on 15.08.2026.
//

#ifndef LIQUIDPETPROJECT_CURRENCY_H
#define LIQUIDPETPROJECT_CURRENCY_H

#include <expected>
#include <stdexcept>
#include <string>


namespace order_system::models {
    class Currency {
    public:
        enum class CurrencyError {
            EmptyCurrency,
            NegativeAmount,
        };

    public:
        static std::expected<Currency, CurrencyError> create(std::string code, int minorDigits) {
            if (minorDigits < 0)
                return std::unexpected(CurrencyError::NegativeAmount);

            if (code.empty())
                return std::unexpected(CurrencyError::EmptyCurrency);

            return Currency{std::move(code), minorDigits};
        }

        const std::string& code() const {
            return _code;
        }

        int minorDigits() const {
            return _minorDigits;
        }

        bool operator ==(const Currency& other) const {
            return _code == other._code;
        }

    private:
        Currency(std::string code, int minorDigits) : _code(std::move(code)), _minorDigits(minorDigits) {
        };

    private:
        std::string _code;
        std::int8_t _minorDigits;
    };
}

#endif //LIQUIDPETPROJECT_CURRENCY_H

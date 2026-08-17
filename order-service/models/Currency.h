//
// Created by DED on 15.08.2026.
//

#ifndef LIQUIDPETPROJECT_CURRENCY_H
#define LIQUIDPETPROJECT_CURRENCY_H

#include <expected>
#include <string>
#include <system_error>

namespace order_system::models {

    enum class CurrencyError {
        EmptyCurrency = 1,
        NegativeAmount,
    };

    class CurrencyErrorCategory : public std::error_category {
    public:
        const char* name() const noexcept override { return "currency"; }

        std::string message(int ev) const override {
            switch (static_cast<CurrencyError>(ev)) {
                case CurrencyError::NegativeAmount:
                    return "negative amount";
                case CurrencyError::EmptyCurrency:
                    return "empty currency";
                default:
                    return "unknown currency error";
            }
        }
    };

    inline const CurrencyErrorCategory& currencyErrorCategory() {
        static CurrencyErrorCategory instance;
        return instance;
    }

    inline std::error_code make_error_code(CurrencyError e) {
        return {static_cast<int>(e), currencyErrorCategory()};
    }

}

namespace std {
    template <>
    struct is_error_code_enum<order_system::models::CurrencyError> : true_type {
    };
}

namespace order_system::models {

    class Currency {
    public:
        static std::expected<Currency, std::error_code> create(std::string code, std::int8_t minorDigits) {
            if (minorDigits < 0)
                return std::unexpected(CurrencyError::NegativeAmount);

            if (code.empty())
                return std::unexpected(CurrencyError::EmptyCurrency);

            return Currency{std::move(code), minorDigits};
        }

        const std::string& code() const { return _code; }
        std::int8_t minorDigits() const { return _minorDigits; }

        bool operator==(const Currency& other) const {
            return _code == other._code;
        }

    private:
        Currency(std::string code, std::int8_t minorDigits) : _code(std::move(code)), _minorDigits(minorDigits) {
        }

    private:
        std::string _code;
        std::int8_t _minorDigits;
    };

}

#endif

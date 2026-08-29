//
// Created by DED on 29.08.2026.
//

#ifndef LIQUIDPETPROJECT_CURRENCYJSONMAPPER_H
#define LIQUIDPETPROJECT_CURRENCYJSONMAPPER_H

#include <json/Json.h>
#include <models/Currency.h>
#include <models2json-mapper/keys/CurrencyJsonKeys.h>

namespace order_system::models2json_mapper {
    using namespace order_system::models;
    using namespace Json;
    using namespace models2json_mapper::keys;

    class CurrencyJsonMapper {
    public:
        static std::expected<Currency, std::error_code> fromJson(const nlohmann::json& json) {
            return JsonHelper::getValue<std::string>(json, CURRENCY_CODE)
                    .and_then([&json](const std::string& code) {
                        return JsonHelper::getValue<std::int8_t>(json, CURRENCY_MINOR_DIGITS)
                                .and_then([&code](const std::int8_t minorDigits) {
                                    return Currency::create(code, minorDigits);
                                });
                    });
        }

        static nlohmann::json toJson(const Currency& currency) {
            nlohmann::json json;
            json[CURRENCY_CODE] = currency.code();
            json[CURRENCY_MINOR_DIGITS] = currency.minorDigits();
            return json;
        }
    };
}

#endif //LIQUIDPETPROJECT_CURRENCYJSONMAPPER_H

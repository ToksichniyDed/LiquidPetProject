//
// Created by DED on 28.08.2026.
//

#ifndef LIQUIDPETPROJECT_MONEYJSONMAPPER_H
#define LIQUIDPETPROJECT_MONEYJSONMAPPER_H

#include <json/Json.h>
#include <models/Money.h>
#include <models2json-mapper/keys/MoneyJsonKeys.h>

namespace order_system::models2json_mapper {
    using namespace order_system::models;
    using namespace Json;
    using namespace models2json_mapper::keys;

    class MoneyJsonMapper {
    public:
        static std::expected<Money, std::error_code> fromJson(const nlohmann::json& json) {
            return JsonHelper::getValue<std::int64_t>(json, keys::MONEY_MINOR_UNITS)
                    .and_then([&json](std::int64_t minorUnits) {
                        return JsonHelper::getValue<nlohmann::json>(json, keys::MONEY_CURRENCY)
                                .and_then([minorUnits](const nlohmann::json& currencyJson) {
                                    return CurrencyJsonMapper::fromJson(currencyJson)
                                            .and_then([minorUnits](Currency currency) {
                                                return Money::create(minorUnits, std::move(currency));
                                            });
                                });
                    });
        }

        static nlohmann::json toJson(const Money& money) {
            nlohmann::json json;
            json[MONEY_MINOR_UNITS] = money.minorUnits();
            json[MONEY_CURRENCY] = CurrencyJsonMapper::toJson(money.currency());
            return json;
        }
    };

}

#endif //LIQUIDPETPROJECT_MONEYJSONMAPPER_H

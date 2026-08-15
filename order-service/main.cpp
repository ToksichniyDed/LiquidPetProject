//
// Created by DED on 15.08.2026.
//

#include <print>

#include "models/Money.h"
#include "models/Currency.h"

int main() {
    auto currency = order_system::models::Currency::create("RUB", 2);
    if (!currency) {
        std::println("Failed to create currency");
        return 1;
    }

    auto money = order_system::models::Money::create(15000, *currency);
    if (!money) {
        std::println("Failed to create money");
        return 1;
    }

    std::println("order-service placeholder: {} {}",
                 money->minorUnits(), money->currency().code());

    return 0;
}
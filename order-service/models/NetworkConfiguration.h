//
// Created by DED on 23.08.2026.
//

#ifndef LIQUIDPETPROJECT_NETWORKCONFIGURATION_H
#define LIQUIDPETPROJECT_NETWORKCONFIGURATION_H

#include <expected>

#include <nlohmann/json.hpp>

#include <NetworkAddress.h>
#include <json/Json.h>

namespace order_system::models {
    struct NetworkConfiguration {
        NetworkAddress address;
        uint16_t port;
    };
}

#endif //LIQUIDPETPROJECT_NETWORKCONFIGURATION_H

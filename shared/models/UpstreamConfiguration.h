//
// Created by DED on 17.09.2026.
//

#ifndef LIQUIDPETPROJECT_UPSTREAMCONFIGURATION_H
#define LIQUIDPETPROJECT_UPSTREAMCONFIGURATION_H

#include <cstdint>

#include "Host.h"

namespace shared::models {

// Конфигурация для подключения к апстриму (например, ProxyHandler -> order-service).
// Сознательно отдельный тип от NetworkConfiguration: тот описывает,
// на чём слушать (bind), этот - куда подключаться (connect). Их нельзя
// перепутать местами, не получив ошибку компиляции.
struct UpstreamConfiguration {
    Host host;
    uint16_t port;
};

}

#endif //LIQUIDPETPROJECT_UPSTREAMCONFIGURATION_H

//
// Created by DED on 15.08.2026.
//

#ifndef LIQUIDPETPROJECT_ORDER_H
#define LIQUIDPETPROJECT_ORDER_H

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace order_system::models {

    enum class OrderStatus {
        Created,
        Reserved,
        Shipped,
        Cancelled
    };
}

#endif //LIQUIDPETPROJECT_ORDER_H

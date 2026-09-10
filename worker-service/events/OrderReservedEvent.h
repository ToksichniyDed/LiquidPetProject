//
// Created by DED on 10.09.2026.
//

#ifndef LIQUIDPETPROJECT_ORDERRESERVEDEVENT_H
#define LIQUIDPETPROJECT_ORDERRESERVEDEVENT_H

#include <models/OrderIds.h>

#include <string>

namespace worker_service::events {
struct OrderReservedEvent {
    shared::models::OrderId orderId;
};
}  // namespace worker_service::events

#endif  // LIQUIDPETPROJECT_ORDERRESERVEDEVENT_H

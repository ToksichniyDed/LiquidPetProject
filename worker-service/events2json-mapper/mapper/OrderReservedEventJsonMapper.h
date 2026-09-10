//
// Created by DED on 10.09.2026.
//

#ifndef LIQUIDPETPROJECT_ORDERRESERVEDEVENTJSONMAPPER_H
#define LIQUIDPETPROJECT_ORDERRESERVEDEVENTJSONMAPPER_H


#include "OrderReservedEvent.h"
#include "keys/OrderCreatedEventJsonKeys.h"

#include <json/Json.h>

namespace worker_service::events {

class OrderReservedEventJsonMapper
{
public:
    static nlohmann::json toJson(const events::OrderReservedEvent& event) {
        nlohmann::json json;
        json[keys::ORDER_ID] = event.orderId.value();

        return json;
    }
};

}

#endif  // LIQUIDPETPROJECT_ORDERRESERVEDEVENTJSONMAPPER_H

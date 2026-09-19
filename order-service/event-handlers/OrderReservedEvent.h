//
// Created by DED on 19.09.2026.
//

#ifndef LIQUIDPETPROJECT_ORDERRESERVEDEVENT_H
#define LIQUIDPETPROJECT_ORDERRESERVEDEVENT_H

#include <models/OrderIds.h>

namespace order_service::event_handlers {

    struct OrderReservedEvent {
        shared::models::OrderId orderId;
    };

}

#endif //LIQUIDPETPROJECT_ORDERRESERVEDEVENT_H

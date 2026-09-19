//
// Created by DED on 19.09.2026.
//

#ifndef LIQUIDPETPROJECT_ORDERRESERVEDHANDLER_H
#define LIQUIDPETPROJECT_ORDERRESERVEDHANDLER_H

#include <IOrderRepository.h>
#include <IOrderStatusRepository.h>
#include <consumer/IEventHandler.h>

namespace order_service::event_handlers {

    class OrderReservedHandler : public shared::messaging::IEventHandler {
    public:
        OrderReservedHandler(order_system::repository::IOrderRepository& orderRepository,
                             order_system::repository::IOrderStatusRepository& orderStatusRepository);

        bool handle(const std::string& payload, const shared::messaging::MessageMetadata& metadata) override;

    private:
        order_system::repository::IOrderRepository& _orderRepository;
        order_system::repository::IOrderStatusRepository& _orderStatusRepository;
    };

}

#endif //LIQUIDPETPROJECT_ORDERRESERVEDHANDLER_H

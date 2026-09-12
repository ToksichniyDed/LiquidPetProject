//
// Created by DED on 10.09.2026.
//

#ifndef LIQUIDPETPROJECT_ORDERRESERVATIONHANDLER_H
#define LIQUIDPETPROJECT_ORDERRESERVATIONHANDLER_H

#include "IWorkerRepository.h"
#include "IOrderProcessor.h"

#include <consumer/IEventHandler.h>
#include <outbox/IOutboxRepository.h>

namespace worker_service::handlers {
class OrderReservationHandler : public shared::messaging::IEventHandler{
public:
    OrderReservationHandler(repository::IWorkerRepository& workerRepository,
                             processing::IOrderProcessor& orderProcessor);
    ~OrderReservationHandler() override = default;

    bool handle(const std::string& payload, const shared::messaging::MessageMetadata& metadata) override;

private:
    repository::IWorkerRepository& _workerRepository;
    processing::IOrderProcessor& _orderProcessor;
};
}



#endif //LIQUIDPETPROJECT_ORDERRESERVATIONHANDLER_H

//
// Created by DED on 10.09.2026.
//

#ifndef LIQUIDPETPROJECT_ORDERRESERVATIONPROCESSOR_H
#define LIQUIDPETPROJECT_ORDERRESERVATIONPROCESSOR_H

#include <chrono>

#include "IOrderProcessor.h"

namespace worker_service::processing {

class OrderReservationProcessor : public IOrderProcessor {
public:
    explicit OrderReservationProcessor(std::chrono::milliseconds simulatedDelay = std::chrono::milliseconds(200));

    [[nodiscard]] std::expected<void, std::error_code> process(const events::OrderCreatedEvent& event) override;

private:
    std::chrono::milliseconds _simulatedDelay;
};

}


#endif //LIQUIDPETPROJECT_ORDERRESERVATIONPROCESSOR_H

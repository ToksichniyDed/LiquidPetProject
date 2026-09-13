//
// Created by DED on 13.09.2026.
//

#ifndef LIQUIDPETPROJECT_MOCKORDERPROCESSOR_H
#define LIQUIDPETPROJECT_MOCKORDERPROCESSOR_H

#include <gmock/gmock.h>

#include "IOrderProcessor.h"

namespace worker_service::processing {

class MockOrderProcessor : public IOrderProcessor {
   public:
    MOCK_METHOD((std::expected<void, std::error_code>), process, (const events::OrderCreatedEvent& event), (override));
};

}  // namespace worker_service::processing

#endif  // LIQUIDPETPROJECT_MOCKORDERPROCESSOR_H

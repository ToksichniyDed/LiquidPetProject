//
// Created by DED on 06.09.2026.
//

#ifndef LIQUIDPETPROJECT_MOCKEVENTPUBLISHER_H
#define LIQUIDPETPROJECT_MOCKEVENTPUBLISHER_H

#include <gmock/gmock.h>

#include "IEventPublisher.h"

namespace order_service::messaging {

    class MockEventPublisher : public IEventPublisher {
    public:
        MOCK_METHOD((std::expected<void, std::error_code>), publish,
                    (const std::string& topic, const std::string& key, const std::string& payload), (override));
    };

}

#endif //LIQUIDPETPROJECT_MOCKEVENTPUBLISHER_H

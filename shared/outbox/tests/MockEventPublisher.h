//
// Created by DED on 06.09.2026.
//

#ifndef LIQUIDPETPROJECT_MOCKEVENTPUBLISHER_H
#define LIQUIDPETPROJECT_MOCKEVENTPUBLISHER_H

#include <gmock/gmock.h>

#include "../../messaging/producer/IEventPublisher.h"

namespace shared::messaging {

    class MockEventPublisher : public IEventPublisher {
    public:
        MOCK_METHOD((std::future<std::expected<void, std::error_code>>), publish,
                    (const std::string& topic, const std::string& key, const std::string& payload), (override));
    };

    inline std::future<std::expected<void, std::error_code>> makeReadyFuture(
        std::expected<void, std::error_code> value) {
        std::promise<std::expected<void, std::error_code>> promise;
        promise.set_value(std::move(value));
        return promise.get_future();
    }

}

#endif //LIQUIDPETPROJECT_MOCKEVENTPUBLISHER_H

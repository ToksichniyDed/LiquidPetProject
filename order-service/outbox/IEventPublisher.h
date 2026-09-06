//
// Created by DED on 06.09.2026.
//

#ifndef LIQUIDPETPROJECT_IEVENTPUBLISHER_H
#define LIQUIDPETPROJECT_IEVENTPUBLISHER_H

#include <string>

namespace order_service::outbox {
    class IEventPublisher {
    public:
        virtual ~IEventPublisher() = default;

        virtual bool publish(const std::string& topic, const std::string& key, const std::string& payload) = 0;
    };
}


#endif //LIQUIDPETPROJECT_IEVENTPUBLISHER_H

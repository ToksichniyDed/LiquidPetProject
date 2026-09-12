//
// Created by DED on 07.09.2026.
//

#ifndef LIQUIDPETPROJECT_KAFKAEVENTPUBLISHER_H
#define LIQUIDPETPROJECT_KAFKAEVENTPUBLISHER_H

#include <future>
#include <memory>
#include <string>

#include "IEventPublisher.h"
#include "messaging/PublishRequest.h"

namespace shared::messaging {

class KafkaEventPublisher : public IEventPublisher {
    public:
        explicit KafkaEventPublisher(const std::string& brokers);
        ~KafkaEventPublisher() override;

        KafkaEventPublisher(const KafkaEventPublisher&) = delete;
        KafkaEventPublisher& operator=(const KafkaEventPublisher&) = delete;

        [[nodiscard]] std::future<std::expected<void, std::error_code>> publish(const PublishRequest& request) override;

    private:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

}


#endif //LIQUIDPETPROJECT_KAFKAEVENTPUBLISHER_H

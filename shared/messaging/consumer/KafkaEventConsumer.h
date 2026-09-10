//
// Created by DED on 08.09.2026.
//

#ifndef LIQUIDPETPROJECT_KAFKAEVENTCONSUMER_H
#define LIQUIDPETPROJECT_KAFKAEVENTCONSUMER_H

#include <memory>

#include "IEventConsumer.h"

namespace shared::messaging {
    class KafkaEventConsumer : public IEventConsumer {
       public:
        explicit KafkaEventConsumer(const std::string& brokers, const std::string& groupId);
        ~KafkaEventConsumer();

        KafkaEventConsumer(const KafkaEventConsumer&) = delete;
        KafkaEventConsumer& operator=(const KafkaEventConsumer&) = delete;

        std::expected<void, std::error_code> start(IEventHandler& handler) override;
        void stop() override;

       private:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };
}  // namespace shared::messaging

#endif  // LIQUIDPETPROJECT_KAFKAEVENTCONSUMER_H

//
// Created by DED on 08.09.2026.
//

#ifndef LIQUIDPETPROJECT_KAFKAEVENTCONSUMER_H
#define LIQUIDPETPROJECT_KAFKAEVENTCONSUMER_H

#include <memory>
#include <chrono>

#include "IEventConsumer.h"
#include <models/KafkaConsumerConfiguration.h>

namespace shared::messaging {
    class KafkaEventConsumer : public IEventConsumer {
       public:
        explicit KafkaEventConsumer(const KafkaConsumerConfiguration& configuration);
        ~KafkaEventConsumer();

        KafkaEventConsumer(const KafkaEventConsumer&) = delete;
        KafkaEventConsumer& operator=(const KafkaEventConsumer&) = delete;

        std::expected<void, std::error_code> start(IEventHandler& handler) override;
        void stop() override;

        // Kafka может быть ещё не готова к старту сервиса: пробуем создать consumer несколько раз
        [[nodiscard]] static std::expected<std::unique_ptr<KafkaEventConsumer>, std::error_code> createWithRetry(
            const KafkaConsumerConfiguration& configuration, int maxAttempts, std::chrono::milliseconds retryDelay);

       private:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };
}  // namespace shared::messaging

#endif  // LIQUIDPETPROJECT_KAFKAEVENTCONSUMER_H

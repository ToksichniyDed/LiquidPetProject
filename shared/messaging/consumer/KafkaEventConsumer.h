//
// Created by DED on 08.09.2026.
//

#ifndef LIQUIDPETPROJECT_KAFKAEVENTCONSUMER_H
#define LIQUIDPETPROJECT_KAFKAEVENTCONSUMER_H

#include <chrono>
#include <memory>

#include <ConsumerPipeline.h>
#include <IDeadLetterSink.h>
#include <models/KafkaConsumerConfiguration.h>

#include "IEventConsumer.h"

namespace shared::messaging {
//handler вызывается одновременно из нескольких потоков пула и должен быть потокобезопасным.
class KafkaEventConsumer : public IEventConsumer {
public:
    KafkaEventConsumer(const KafkaConsumerConfiguration& configuration,
                       std::shared_ptr<IDeadLetterSink> deadLetterSink,
                       const PipelineConfiguration& pipelineConfig = {});
    ~KafkaEventConsumer() override;

    KafkaEventConsumer(const KafkaEventConsumer&) = delete;
    KafkaEventConsumer& operator=(const KafkaEventConsumer&) = delete;

    std::expected<void, std::error_code> start(IEventHandler& handler) override;
    void stop() override;

    [[nodiscard]] static std::expected<std::unique_ptr<KafkaEventConsumer>, std::error_code> createWithRetry(
        const KafkaConsumerConfiguration& configuration, std::shared_ptr<IDeadLetterSink> deadLetterSink,
        int maxAttempts, std::chrono::milliseconds retryDelay, const PipelineConfiguration& pipelineConfig = {});

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};
}

#endif  // LIQUIDPETPROJECT_KAFKAEVENTCONSUMER_H

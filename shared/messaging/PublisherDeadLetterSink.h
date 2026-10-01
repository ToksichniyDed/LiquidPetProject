//
// Created by DED on 01.10.2026.
//

#ifndef LIQUIDPETPROJECT_PUBLISHERDEADLETTERSINK_H
#define LIQUIDPETPROJECT_PUBLISHERDEADLETTERSINK_H

#include <chrono>
#include <future>
#include <memory>
#include <string>

#include "IDeadLetterSink.h"
#include "producer/IEventPublisher.h"

namespace shared::messaging {

static constexpr std::string kTopicSuffix = ".dlq";

// Кладёт запись в топик "<исходный топик><suffix>" с исходным payload, eventId и eventType.
// Откуда и сколько раз падало (партиция, оффсет, attempts) пока живёт только в логах.
class PublisherDeadLetterSink : public IDeadLetterSink {
public:
    explicit PublisherDeadLetterSink(std::shared_ptr<IEventPublisher> publisher, std::string topicSuffix = kTopicSuffix,
                                     std::chrono::milliseconds timeout = std::chrono::milliseconds(5000))
        : _publisher(std::move(publisher)), _topicSuffix(std::move(topicSuffix)), _timeout(timeout) {}

    [[nodiscard]] std::expected<void, std::error_code> send(const DeadLetterRecord& record) override {
        auto result = _publisher->publish(PublishRequest{
            .topic = record.source.topic + _topicSuffix,
            .key = record.metadata.eventId.value(),
            .payload = record.payload,
            .metadata = record.metadata,
        });

        if (result.wait_for(_timeout) != std::future_status::ready)
            return std::unexpected(EventPublisherError::Timeout);

        return result.get();
    }

private:
    std::shared_ptr<IEventPublisher> _publisher;
    std::string _topicSuffix;
    std::chrono::milliseconds _timeout;
};

}

#endif  // LIQUIDPETPROJECT_PUBLISHERDEADLETTERSINK_H

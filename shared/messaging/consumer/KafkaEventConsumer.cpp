//
// Created by DED on 08.09.2026.
//

#include "KafkaEventConsumer.h"

#include <kafka/KafkaConsumer.h>
#include <logging/Logger.h>
#include <messaging/MessageMetadata.h>

namespace shared::messaging
{
    using namespace kafka;
    using namespace kafka::clients::consumer;

    class KafkaEventConsumer::Impl
    {
    public:
        explicit Impl(const KafkaConsumerConfiguration& configuration) : _consumer(makeProperties(configuration))
        {
            _consumer.subscribe({configuration.topic});

            SPDLOG_LOGGER_INFO(shared::logger::get("KafkaEventConsumer"), "Kafka consumer created successfully!");
            SPDLOG_LOGGER_INFO(shared::logger::get("KafkaEventConsumer"), "Kafka brokers: {}", configuration.brokers);
            SPDLOG_LOGGER_INFO(shared::logger::get("KafkaEventConsumer"), "Subscribed to topic: {}", configuration.topic);
        }

        // true  - запись закрыта: обработана и закоммичена, либо осознанно пропущена (битые метаданные)
        // false - обработчик просит повторить: оффсет не коммитим, вызывающий обязан вернуться к записи
        bool processRecord(const ConsumerRecord& record, IEventHandler& handler)
        {
            const std::string payload(static_cast<const char*>(record.value().data()), record.value().size());

            std::string eventIdRaw;
            std::string eventType;
            for (const auto& header : record.headers())
            {
                const std::string headerValue(static_cast<const char*>(header.value.data()), header.value.size());

                if (header.key == "eventId")
                    eventIdRaw = headerValue;
                else if (header.key == "eventType")
                    eventType = headerValue;
            }

            auto eventIdResult = models::OutboxEventId::create(eventIdRaw);
            if (!eventIdResult.has_value())
            {
                SPDLOG_LOGGER_ERROR(shared::logger::get("KafkaEventConsumer"),
                                    "Message missing valid eventId header, skipping (offset will be committed)");
                commit(record); // битые метаданные не ретраить бесконечно
                return true;
            }


            if (const MessageMetadata metadata{
                .eventId = std::move(eventIdResult.value()),
                .eventType = eventType,
            }; !handler.handle(payload, metadata))
            {
                SPDLOG_LOGGER_WARN(shared::logger::get("KafkaEventConsumer"),
                                   "Handler failed for {}-{}@{}, will rewind and retry",
                                   record.topic(), record.partition(), record.offset());
                return false;
            }

            commit(record);
            return true;
        }

        // Возвращает true, если хотя бы одна партиция откатывалась назад (нужна пауза перед повтором)
        bool processBatch(const std::vector<ConsumerRecord>& records, IEventHandler& handler)
        {
            std::set<TopicPartition> rewoundPartitions;

            for (const auto& record : records)
            {
                if (record.error())
                {
                    SPDLOG_LOGGER_ERROR(shared::logger::get("KafkaEventConsumer"),
                                        "Error while consuming: {}", record.error().message());
                    continue;
                }

                TopicPartition partition{record.topic(), record.partition()};

                // После сбоя остаток пачки для этой партиции не трогаем
                if (rewoundPartitions.contains(partition))
                    continue;

                if (!processRecord(record, handler))
                {
                    _consumer.seek(partition, record.offset());
                    rewoundPartitions.insert(std::move(partition));
                }
            }

            return !rewoundPartitions.empty();
        }

        void commit(const ConsumerRecord& record)
        {
            try
            {
                _consumer.commitSync(record);
            }
            catch (const KafkaException& e)
            {
                // Запись обработана, просто не закоммичена. Она может прийти повторно после ребаланса
                // или рестарта, дубликат отсечёт идемпотентность обработчика. Следующий успешный
                // коммит покроет и этот оффсет.
                SPDLOG_LOGGER_WARN(shared::logger::get("KafkaEventConsumer"),
                                   "Commit failed for {}-{}@{}: {}",
                                   record.topic(), record.partition(), record.offset(), e.what());
            }
        }

    public:
        KafkaConsumer _consumer;
        std::jthread _thread;

    private:
        static Properties makeProperties(const KafkaConsumerConfiguration& configuration)
        {
            Properties props;
            props.put("bootstrap.servers", configuration.brokers);
            props.put("group.id", configuration.groupId);
            props.put("enable.auto.commit", "false");
            props.put("auto.offset.reset", "earliest");
            return props;
        }
    };

    namespace
    {
        constexpr auto POLL_TIMEOUT = std::chrono::milliseconds(100);
        constexpr auto RETRY_BACKOFF = std::chrono::milliseconds(500);
    }

    KafkaEventConsumer::KafkaEventConsumer(const KafkaConsumerConfiguration& configuration) : _impl(std::make_unique<Impl>(configuration))
    {
    }

    KafkaEventConsumer::~KafkaEventConsumer() = default;

    std::expected<void, std::error_code> KafkaEventConsumer::start(IEventHandler& handler)
    {
        _impl->_thread = std::jthread([this, &handler](std::stop_token stopToken)
        {
            while (!stopToken.stop_requested())
            {
                try
                {
                    const auto records = _impl->_consumer.poll(POLL_TIMEOUT);

                    if (_impl->processBatch(records, handler))
                        std::this_thread::sleep_for(RETRY_BACKOFF);
                }
                catch (const KafkaException& e)
                {
                    SPDLOG_LOGGER_ERROR(shared::logger::get("KafkaEventConsumer"),
                                        "Kafka error in consumer loop: {}", e.what());
                    std::this_thread::sleep_for(RETRY_BACKOFF);
                }
            }
        });

        return {};
    }

    void KafkaEventConsumer::stop()
    {
        SPDLOG_LOGGER_INFO(shared::logger::get("KafkaEventConsumer"), "Stop kafka consumer");
        _impl->_thread.request_stop();
    }

} // namespace shared::messaging

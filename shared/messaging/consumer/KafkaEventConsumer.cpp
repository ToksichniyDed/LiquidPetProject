//
// Created by DED on 08.09.2026.
//

#include "KafkaEventConsumer.h"

#include <kafka/KafkaConsumer.h>
#include <logging/Logger.h>

namespace shared::messaging
{
    using namespace kafka;
    using namespace kafka::clients::consumer;

    class KafkaEventConsumer::Impl
    {
    public:
        explicit Impl(const std::string& brokers, const std::string& groupId) : _consumer(makeProperties(brokers, groupId))
        {
            SPDLOG_LOGGER_INFO(shared::logger::get("KafkaEventConsumer"), "Kafka consumer created successfully!");
            SPDLOG_LOGGER_INFO(shared::logger::get("KafkaEventConsumer"), "Kafka brokers: {}", brokers);
        }

        void processRecord(const ConsumerRecord& record, IEventHandler& handler)
        {
            if (record.error())
            {
                SPDLOG_LOGGER_ERROR(shared::logger::get("KafkaEventConsumer"),
                                     "Error while consuming: {}", record.error().message());
                return;
            }

            const std::string payload(static_cast<const char*>(record.value().data()), record.value().size());

            if (handler.handle(payload))
            {
                _consumer.commitSync(record);
            }
            else
            {
                SPDLOG_LOGGER_WARN(shared::logger::get("KafkaEventConsumer"),
                                    "Handler failed, offset not committed, will retry on next poll");
            }
        }

    public:
        KafkaConsumer _consumer;
        std::jthread _thread;

    private:
        static Properties makeProperties(const std::string& brokers, const std::string& groupId)
        {
            Properties props;
            props.put("bootstrap.servers", brokers);
            props.put("group.id", groupId);
            props.put("enable.auto.commit", "false");
            props.put("auto.offset.reset", "earliest");
            return props;
        }
    };

    namespace
    {
        std::error_code mapError(const Error& error)
        {
            switch (error.value())
            {
            case RD_KAFKA_RESP_ERR__MSG_TIMED_OUT:
                return EventConsumerError::Timeout;
            case RD_KAFKA_RESP_ERR_TOPIC_AUTHORIZATION_FAILED:
            case RD_KAFKA_RESP_ERR_CLUSTER_AUTHORIZATION_FAILED:
            case RD_KAFKA_RESP_ERR_UNKNOWN_TOPIC_OR_PART:
                return EventConsumerError::BrokerRejected;
            default:
                return EventConsumerError::ConnectionFailure;
            }
        }
    } // namespace

    KafkaEventConsumer::KafkaEventConsumer(const std::string& brokers, const std::string& groupId) : _impl(std::make_unique<Impl>(brokers, groupId))
    {
    }

    KafkaEventConsumer::~KafkaEventConsumer() = default;

    std::expected<void, std::error_code> KafkaEventConsumer::start(IEventHandler& handler)
    {
        _impl->_thread = std::jthread([this, &handler](std::stop_token stopToken)
        {
            while (!stopToken.stop_requested())
            {
                auto records = _impl->_consumer.poll(std::chrono::milliseconds(100));

                for (const auto& record : records)
                {
                    _impl->processRecord(record, handler);
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

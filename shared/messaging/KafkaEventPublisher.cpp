//
// Created by DED on 07.09.2026.
//

#include "KafkaEventPublisher.h"

#include <kafka/KafkaProducer.h>
#include <logging/Logger.h>

namespace messaging {
    using namespace kafka;
    using namespace kafka::clients::producer;

    class KafkaEventPublisher::Impl {
    public:
        explicit Impl(const std::string& brokers) : _producer(makeProperties(brokers)) {
            SPDLOG_LOGGER_INFO(Logger::get("KafkaEventPublisher"), "Kafka producer created successfully!");
            SPDLOG_LOGGER_INFO(Logger::get("KafkaEventPublisher"), "Kafka brokers: {}", brokers);
        }

    public:
        KafkaProducer _producer;

    private:
        static Properties makeProperties(const std::string& brokers) {
            Properties props;
            props.put("bootstrap.servers", brokers);
            props.put("enable.idempotence", "true");
            props.put("message.timeout.ms", "5000");
            return props;
        }
    };

    namespace {

        std::error_code mapError(const Error& error) {
            switch (error.value()) {
                case RD_KAFKA_RESP_ERR__MSG_TIMED_OUT:
                    return EventPublisherError::Timeout;
                case RD_KAFKA_RESP_ERR_TOPIC_AUTHORIZATION_FAILED:
                case RD_KAFKA_RESP_ERR_CLUSTER_AUTHORIZATION_FAILED:
                case RD_KAFKA_RESP_ERR_UNKNOWN_TOPIC_OR_PART:
                    return EventPublisherError::BrokerRejected;
                case RD_KAFKA_RESP_ERR__INVALID_ARG:
                    return EventPublisherError::SerializationFailure;
                default:
                    return EventPublisherError::ConnectionFailure;
            }
        }

    }

    KafkaEventPublisher::KafkaEventPublisher(const std::string& brokers) : _impl(std::make_unique<Impl>(brokers)) {
    }

    KafkaEventPublisher::~KafkaEventPublisher() = default;

    std::future<std::expected<void, std::error_code>> KafkaEventPublisher::publish(const std::string& topic,
        const std::string& key, const std::string& payload) {
        auto resultPromise = std::make_shared<std::promise<std::expected<void, std::error_code>>>();
        auto resultFuture = resultPromise->get_future();

        try {
            const ProducerRecord record(
                topic,
                Key{key.c_str(), key.size()},
                Value{payload.c_str(), payload.size()});

            auto deliveryCb = [resultPromise](const RecordMetadata& metadata, const Error& error) {
                if (error) {
                    SPDLOG_LOGGER_WARN(Logger::get("KafkaEventPublisher"),
                                       "Message failed to be delivered: {}", error.message());
                    resultPromise->set_value(std::unexpected(mapError(error)));
                } else {
                    resultPromise->set_value({});
                }
            };

            _impl->_producer.send(record, deliveryCb, KafkaProducer::SendOption::ToCopyRecordValue);
        } catch (const KafkaException& e) {
            SPDLOG_LOGGER_ERROR(Logger::get("KafkaEventPublisher"), "Failed to send message: {}", e.what());
            resultPromise->set_value(std::unexpected(mapError(e.error())));
        }

        return resultFuture;
    }
}

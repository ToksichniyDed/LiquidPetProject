//
// Created by DED on 07.09.2026.
//

#include "KafkaEventPublisher.h"

#include <kafka/KafkaProducer.h>
#include <logging/Logger.h>

namespace shared::messaging {
using namespace kafka;
using namespace kafka::clients::producer;

class KafkaEventPublisher::Impl {
   public:
    explicit Impl(const std::string& brokers) : _producer(makeProperties(brokers)) {
        SPDLOG_LOGGER_INFO(shared::logger::get("KafkaEventPublisher"), "Kafka producer created successfully!");
        SPDLOG_LOGGER_INFO(shared::logger::get("KafkaEventPublisher"), "Kafka brokers: {}", brokers);
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

}  // namespace

KafkaEventPublisher::KafkaEventPublisher(const std::string& brokers) : _impl(std::make_unique<Impl>(brokers)) {}

KafkaEventPublisher::~KafkaEventPublisher() = default;

std::future<std::expected<void, std::error_code>> KafkaEventPublisher::publish(const PublishRequest& request) {
    auto resultPromise = std::make_shared<std::promise<std::expected<void, std::error_code>>>();
    auto resultFuture = resultPromise->get_future();

    try {
        ProducerRecord record(request.topic, Key{request.key.c_str(), request.key.size()},
                              Value{request.payload.c_str(), request.payload.size()});

        record.headers().emplace_back("eventId",Header::Value{request.metadata.eventId.value().c_str(), request.metadata.eventId.value().size()});
        record.headers().emplace_back("eventType", Header::Value{request.metadata.eventType.c_str(), request.metadata.eventType.size()});

        auto deliveryCb = [resultPromise](const RecordMetadata& metadata, const Error& error) {
            if (error) {
                SPDLOG_LOGGER_WARN(shared::logger::get("KafkaEventPublisher"), "Message failed to be delivered: {}",
                                   error.message());
                resultPromise->set_value(std::unexpected(mapError(error)));
            } else {
                resultPromise->set_value({});
            }
        };

        _impl->_producer.send(record, deliveryCb, KafkaProducer::SendOption::ToCopyRecordValue);
    } catch (const KafkaException& e) {
        SPDLOG_LOGGER_ERROR(shared::logger::get("KafkaEventPublisher"), "Failed to send message: {}", e.what());
        resultPromise->set_value(std::unexpected(mapError(e.error())));
    }

    return resultFuture;
}
}  // namespace shared::messaging

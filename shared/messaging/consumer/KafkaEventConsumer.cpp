//
// Created by DED on 08.09.2026.
//

#include "KafkaEventConsumer.h"

#include <kafka/KafkaConsumer.h>
#include <logging/Logger.h>

#include <algorithm>
#include <thread>

namespace shared::messaging {
using namespace kafka;
using namespace kafka::clients::consumer;

namespace {
constexpr auto POLL_TIMEOUT = std::chrono::milliseconds(100);
constexpr auto RETRY_BACKOFF = std::chrono::milliseconds(500);
constexpr auto COMMIT_INTERVAL = std::chrono::milliseconds(500);
constexpr int SUBSCRIBE_ATTEMPTS = 10;
constexpr auto SUBSCRIBE_RETRY_DELAY = std::chrono::seconds(3);

std::vector<PartitionKey> toKeys(const TopicPartitions& partitions) {
    std::vector<PartitionKey> keys;
    keys.reserve(partitions.size());
    for (const auto& [topic, partition] : partitions)
        keys.push_back(PartitionKey{.topic = topic, .partition = partition});
    return keys;
}
} // namespace

class KafkaEventConsumer::Impl {
public:
    Impl(const KafkaConsumerConfiguration& configuration, std::shared_ptr<IDeadLetterSink> deadLetterSink,
         const PipelineConfiguration& pipelineConfig)
        : _configuration(configuration),
          _deadLetterSink(std::move(deadLetterSink)),
          _pipelineConfig(pipelineConfig),
          _consumer(makeProperties(configuration)) {
        SPDLOG_LOGGER_INFO(shared::logger::get("KafkaEventConsumer"), "Kafka consumer created, brokers: {}, topic: {}",
                           configuration.brokers, configuration.topic);
    }

    std::expected<void, std::error_code> start(IEventHandler& handler) {
        if (_pipeline)
            return std::unexpected(std::make_error_code(std::errc::operation_in_progress));

        _pipeline = std::make_unique<ConsumerPipeline>(handler, *_deadLetterSink, _pipelineConfig);

        if (!subscribeWithRetry()) {
            _pipeline.reset();
            return std::unexpected(EventConsumerError::ConnectionFailure);
        }

        _thread = std::jthread([this](const std::stop_token& stopToken) { run(stopToken); });
        return {};
    }

    void stop() {
        if (!_thread.joinable())
            return;

        SPDLOG_LOGGER_INFO(shared::logger::get("KafkaEventConsumer"), "Stop kafka consumer");
        _thread.request_stop();
        _thread.join();

        _pipeline->stop(); // воркеры завершились: все итоговые complete() уже учтены
        commitReady(true); // финальный коммит готового префикса
    }

private:
    bool subscribeWithRetry() {
        for (int attempt = 1; attempt <= SUBSCRIBE_ATTEMPTS; ++attempt) {
            try {
                // Rebalance-колбэки вызываются из poll()/subscribe() в вызывающем потоке
                _consumer.subscribe({_configuration.topic},
                                    [this](RebalanceEventType type, const TopicPartitions& partitions) {
                                        onRebalance(type, partitions);
                                    });
                SPDLOG_LOGGER_INFO(shared::logger::get("KafkaEventConsumer"), "Subscribed to topic: {}",
                                   _configuration.topic);
                return true;
            } catch (const KafkaException& e) {
                SPDLOG_LOGGER_WARN(shared::logger::get("KafkaEventConsumer"), "Subscribe failed ({}/{}): {}", attempt,
                                   SUBSCRIBE_ATTEMPTS, e.what());
                if (attempt < SUBSCRIBE_ATTEMPTS)
                    std::this_thread::sleep_for(SUBSCRIBE_RETRY_DELAY);
            }
        }
        return false;
    }

    void run(const std::stop_token& stopToken) {
        while (!stopToken.stop_requested()) {
            try {
                for (const auto& record : _consumer.poll(POLL_TIMEOUT))
                    ingest(record);

                applyBackpressure(_pipeline->pump());
                commitReady(false);
            } catch (const KafkaException& e) {
                SPDLOG_LOGGER_ERROR(shared::logger::get("KafkaEventConsumer"), "Kafka error in consumer loop: {}",
                                    e.what());
                std::ignore = sleepUnlessStopped(stopToken, RETRY_BACKOFF);
            }
        }
    }

    void ingest(const ConsumerRecord& record) {
        if (record.error()) {
            SPDLOG_LOGGER_ERROR(shared::logger::get("KafkaEventConsumer"), "Error while consuming: {}",
                                record.error().message());
            return;
        }

        IncomingRecord incoming{
            .partition = PartitionKey{.topic = record.topic(), .partition = record.partition()},
            .offset = record.offset(),
            .payload = std::string(static_cast<const char*>(record.value().data()), record.value().size()),
            .eventId = std::nullopt,
            .eventType = {}};

        for (const auto& header : record.headers()) {
            const std::string value(static_cast<const char*>(header.value.data()), header.value.size());
            if (header.key == "eventId")
                incoming.eventId = value;
            else if (header.key == "eventType")
                incoming.eventType = value;
        }

        _pipeline->ingest(std::move(incoming));
    }

    // Пока backlog не пуст, читаем "вхолостую": poll() нужен, чтобы оставаться в группе, но новых записей не берём.
    // pause() повторяется каждый цикл намеренно: после rebalance новые партиции приходят неприостановленными.
    void applyBackpressure(bool drained) {
        if (!drained) {
            _consumer.pause();
            _paused = true;
            return;
        }
        if (_paused) {
            _consumer.resume();
            _paused = false;
        }
    }

    void onRebalance(RebalanceEventType type, const TopicPartitions& partitions) {
        if (!_pipeline)
            return;

        const auto keys = toKeys(partitions);
        if (type == RebalanceEventType::PartitionsAssigned) {
            _pipeline->onPartitionsAssigned(keys);
            return;
        }

        // Последний шанс закоммитить прогресс, пока партицию не забрал другой консьюмер
        std::ignore = commitOffsets(_pipeline->onPartitionsRevoked(keys));
    }

    bool commitOffsets(const std::vector<CommitPosition>& positions) {
        if (positions.empty())
            return true;

        TopicPartitionOffsets offsets;
        for (const auto& position : positions)
            offsets[TopicPartition{position.partition.topic, position.partition.partition}] = position.offset;

        try {
            _consumer.commitSync(offsets);
            return true;
        } catch (const KafkaException& e) {
            // Не страшно: позиция останется кандидатом и уйдёт со следующим коммитом
            SPDLOG_LOGGER_WARN(shared::logger::get("KafkaEventConsumer"), "Commit failed: {}", e.what());
            return false;
        }
    }

    void commitReady(bool force) {
        const auto now = std::chrono::steady_clock::now();
        if (!force && now - _lastCommit < COMMIT_INTERVAL)
            return;

        const auto positions = _pipeline->commitCandidates();
        if (positions.empty())
            return;

        _lastCommit = now;
        if (!commitOffsets(positions))
            return;

        for (const auto& position : positions)
            _pipeline->confirmCommit(position);
    }

    static Properties makeProperties(const KafkaConsumerConfiguration& configuration) {
        Properties props;
        props.put("bootstrap.servers", configuration.brokers);
        props.put("group.id", configuration.groupId);
        props.put("enable.auto.commit", "false");
        props.put("auto.offset.reset", "earliest");
        return props;
    }

private:
    // Порядок важен. Уничтожаются в обратном порядке: поток, затем consumer (его close() может вызвать
    // revoke-колбэк, которому нужен живой pipeline), затем pipeline, затем sink.
    KafkaConsumerConfiguration _configuration;
    std::shared_ptr<IDeadLetterSink> _deadLetterSink;
    PipelineConfiguration _pipelineConfig;
    std::unique_ptr<ConsumerPipeline> _pipeline;
    KafkaConsumer _consumer;
    bool _paused = false;
    std::chrono::steady_clock::time_point _lastCommit{};
    std::jthread _thread;
};

KafkaEventConsumer::KafkaEventConsumer(const KafkaConsumerConfiguration& configuration,
                                       std::shared_ptr<IDeadLetterSink> deadLetterSink,
                                       const PipelineConfiguration& pipelineConfig)
    : _impl(std::make_unique<Impl>(configuration, std::move(deadLetterSink), pipelineConfig)) {
}

KafkaEventConsumer::~KafkaEventConsumer() = default;

std::expected<void, std::error_code> KafkaEventConsumer::start(IEventHandler& handler) {
    return _impl->start(handler);
}

void KafkaEventConsumer::stop() { _impl->stop(); }

std::expected<std::unique_ptr<KafkaEventConsumer>, std::error_code> KafkaEventConsumer::createWithRetry(
    const KafkaConsumerConfiguration& configuration, std::shared_ptr<IDeadLetterSink> deadLetterSink,
    int maxAttempts, std::chrono::milliseconds retryDelay, const PipelineConfiguration& pipelineConfig) {
    for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
        try {
            return std::make_unique<KafkaEventConsumer>(configuration, deadLetterSink, pipelineConfig);
        } catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(shared::logger::get("KafkaEventConsumer"),
                               "Kafka consumer init failed (attempt {}/{}): {}", attempt, maxAttempts, e.what());
            if (attempt < maxAttempts)
                std::this_thread::sleep_for(retryDelay);
        }
    }
    return std::unexpected(EventConsumerError::ConnectionFailure);
}
} // namespace shared::messaging

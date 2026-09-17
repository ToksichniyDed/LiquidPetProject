//
// Created by DED on 08.09.2026.
//

#include <logging/Logger.h>
#include <messaging/consumer/KafkaEventConsumer.h>
#include <messaging/producer/KafkaEventPublisher.h>
#include <models/DatabaseConfiguration.h>
#include <models/EnvironmentConfiguration.h>
#include <outbox/OutboxPublisher.h>
#include <outbox/PostgresOutboxRepository.h>

#include <atomic>
#include <csignal>
#include <cstdlib>

#include "handlers/OrderReservationHandler.h"
#include "processing/OrderReservationProcessor.h"
#include "repository/PostgresWorkerRepository.h"

namespace {

std::atomic<bool> g_shutdownRequested{false};

void handleShutdownSignal(int) { g_shutdownRequested.store(true); }

template <typename T>
T unwrapOrExit(std::expected<T, std::error_code> result) {
    if (!result.has_value()) {
        SPDLOG_LOGGER_CRITICAL(shared::logger::get("main"), "Error {} : {}", result.error().category().name(),
                               result.error().message());
        std::exit(1);
    }
    return std::move(result.value());
}

shared::models::EnvironmentSchema workerServiceEnvironmentSchema() {
    return {
        {.name = "WORKER_SERVICE_DATABASE_URL", .required = true},
        {.name = "WORKER_SERVICE_KAFKA_BROKERS", .required = true},
    };
}

}  // namespace

int main() {
    std::signal(SIGINT, handleShutdownSignal);
    std::signal(SIGTERM, handleShutdownSignal);

    shared::logger::init(true, false, spdlog::level::level_enum::debug, {}, 1024, 0);

    auto environment = unwrapOrExit(shared::models::EnvironmentConfiguration::load(workerServiceEnvironmentSchema()));

    const auto& databaseUrl = environment.require("WORKER_SERVICE_DATABASE_URL");
    const auto& kafkaBrokers = environment.require("WORKER_SERVICE_KAFKA_BROKERS");

    auto databaseConfiguration = unwrapOrExit(shared::models::DatabaseConfiguration::fromUrl(databaseUrl));

    std::shared_ptr<worker_service::repository::IWorkerRepository> workerRepository;
    std::shared_ptr<shared::outbox::IOutboxRepository> outboxRepository;

    try {
        workerRepository =
            std::make_shared<worker_service::repository::PostgresWorkerRepository>(databaseConfiguration);
        outboxRepository = std::make_shared<shared::outbox::PostgresOutboxRepository>(databaseConfiguration);
    } catch (const std::exception& e) {
        SPDLOG_LOGGER_CRITICAL(shared::logger::get("main"), "Error: {}", e.what());
        return 1;
    }

    std::shared_ptr<shared::messaging::IEventPublisher> eventPublisher;
    try {
        eventPublisher = std::make_shared<shared::messaging::KafkaEventPublisher>(kafkaBrokers);
    } catch (const std::exception& e) {
        SPDLOG_LOGGER_CRITICAL(shared::logger::get("main"), "Error creating Kafka producer: {}", e.what());
        return 1;
    }

    shared::outbox::OutboxPublisher outboxPublisher(outboxRepository, eventPublisher, "orders.reserved");
    outboxPublisher.start();

    worker_service::processing::OrderReservationProcessor orderProcessor;
    worker_service::handlers::OrderReservationHandler handler(*workerRepository, orderProcessor);

    std::optional<shared::messaging::KafkaEventConsumer> consumer;

    constexpr int maxRetries = 10;
    constexpr auto retryDelay = std::chrono::seconds(3);

    for (int attempt = 1; attempt <= maxRetries; ++attempt) {
        try {
            consumer.emplace(shared::messaging::KafkaConsumerConfiguration{
                .brokers = kafkaBrokers,
                .groupId = "worker-service-group",
                .topic = "orders.created",
            });
            break;
        } catch (const std::exception& e) {
            SPDLOG_LOGGER_WARN(shared::logger::get("main"),
                               "Kafka consumer init failed (attempt {}/{}): {}", attempt, maxRetries, e.what());
            if (attempt == maxRetries) {
                SPDLOG_LOGGER_CRITICAL(shared::logger::get("main"), "Giving up after {} attempts", maxRetries);
                outboxPublisher.stop();
                return 1;
            }
            std::this_thread::sleep_for(retryDelay);
        }
    }

    if (auto startResult = consumer->start(handler); !startResult.has_value()) {
        SPDLOG_LOGGER_CRITICAL(
            shared::logger::get("main"),
            "Failed to start Kafka consumer: {}",
            startResult.error().message()
        );

        outboxPublisher.stop();
        return 1;
    }

    SPDLOG_LOGGER_INFO(shared::logger::get("main"), "Worker service started");

    while (!g_shutdownRequested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    SPDLOG_LOGGER_INFO(shared::logger::get("main"), "Shutdown requested, stopping consumer");
    consumer.value().stop();

    SPDLOG_LOGGER_INFO(shared::logger::get("main"), "Stopping outbox publisher");
    outboxPublisher.stop();

    return 0;
}

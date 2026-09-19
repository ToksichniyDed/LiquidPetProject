//
// Created by DED on 15.08.2026.
//

#include <CQRS/CreateOrderHandler.h>
#include <CQRS/GetOrderHandler.h>
#include <CQRS/HealthHandler.h>
#include <IOrderRepository.h>
#include <OrderReservedHandler.h>
#include <PostgresOrderRepository.h>
#include <RoutePaths.h>
#include <http/HttpServer.h>
#include <http/Route.h>
#include <logging/Logger.h>
#include <messaging/producer/KafkaEventPublisher.h>
#include <messaging/consumer/KafkaEventConsumer.h>
#include <models/NetworkConfiguration.h>
#include <models/EnvironmentConfiguration.h>
#include <models2json-mapper/mapper/NetworkConfigurationJsonMapper.h>
#include <outbox/OutboxPublisher.h>
#include <outbox/PostgresOutboxRepository.h>

#include <CLI/CLI.hpp>
#include <expected>
#include <filesystem>

namespace {

    namespace {

        struct CliOptions {
            std::filesystem::path configPath;
            std::optional<std::string> addressOverride;
            std::optional<std::uint16_t> portOverride;
        };

        void setupCliOptions(CLI::App& app, CliOptions& options) {
            app.add_option("-c,--config", options.configPath, "Path to config file")
               ->required()
               ->check(CLI::ExistingFile);
            app.add_option("--address", options.addressOverride, "Override network address from config");
            app.add_option("--port", options.portOverride, "Override network port from config");
        }

    shared::models::EnvironmentSchema orderServiceEnvironmentSchema() {
            return {
                {.name = "ORDER_SERVICE_DATABASE_URL", .required = true},
                {.name = "ORDER_SERVICE_KAFKA_BROKERS", .required = true},
            };
        }

    }

    std::expected<shared::models::NetworkConfiguration, std::error_code> loadNetworkConfiguration(
        const std::filesystem::path& configPath,
        const std::optional<std::string>& addressOverride,
        const std::optional<std::uint16_t>& portOverride) {

        auto networkSection = shared::json::JsonHelper::loadSection(configPath, "network");
        if (!networkSection.has_value()) {
            return std::unexpected(networkSection.error());
        }

        auto networkConfiguration = shared::models2json_mapper::NetworkConfigurationJsonMapper::fromJson(
            networkSection.value());
        if (!networkConfiguration.has_value()) {
            return std::unexpected(networkConfiguration.error());
        }

        if (addressOverride.has_value()) {
            auto overriddenAddress = shared::models::NetworkAddress::create(*addressOverride);
            if (!overriddenAddress.has_value()) {
                return std::unexpected(overriddenAddress.error());
            }
            networkConfiguration.value().address = overriddenAddress.value();
            SPDLOG_LOGGER_INFO(shared::logger::get("main"), "Network address overridden via CLI: {}", *addressOverride);
        }

        if (portOverride.has_value()) {
            networkConfiguration.value().port = *portOverride;
            SPDLOG_LOGGER_INFO(shared::logger::get("main"), "Network port overridden via CLI: {}", *portOverride);
        }

        return networkConfiguration.value();
    }

    std::vector<shared::http::handlers::Route> buildRoutes(
        const std::shared_ptr<order_system::repository::IOrderRepository>& repository) {

        return {
            {
                .method = shared::models::Method::Post,
                .pathPrefix = order_service::handlers::paths::ORDERS,
                .handler = std::make_shared<order_service::handlers::CreateOrderHandler>(repository)
            },
            {
                .method = shared::models::Method::Get,
                .pathPrefix = order_service::handlers::paths::ORDERS_PREFIX,
                .handler = std::make_shared<order_service::handlers::GetOrderHandler>(repository)
            },
            {
                .method = shared::models::Method::Get,
                .pathPrefix = order_service::handlers::paths::HEALTH,
                .handler = std::make_shared<order_service::handlers::HealthHandler>()
            },
        };
    }

    template <typename T>
    T unwrapOrExit(std::expected<T, std::error_code> result) {
        if (!result.has_value()) {
            SPDLOG_LOGGER_CRITICAL(shared::logger::get("main"), "Error {} : {}",
                                   result.error().category().name(), result.error().message());
            std::exit(1);
        }
        return std::move(result.value());
    }

}

int main(const int argc, char* argv[]) {
    CLI::App app{"Order Service"};
    CliOptions options;
    setupCliOptions(app, options);

    CLI11_PARSE(app, argc, argv);

    shared::logger::init(true, false, spdlog::level::level_enum::debug, {}, 1024, 0);

auto environment = unwrapOrExit(
    shared::models::EnvironmentConfiguration::load(orderServiceEnvironmentSchema()));

const auto& databaseUrl = environment.require("ORDER_SERVICE_DATABASE_URL");
const auto& kafkaBrokers = environment.require("ORDER_SERVICE_KAFKA_BROKERS");

auto networkConfiguration = unwrapOrExit(
    loadNetworkConfiguration(options.configPath, options.addressOverride, options.portOverride));

auto databaseConfiguration = unwrapOrExit(
    shared::models::DatabaseConfiguration::fromUrl(databaseUrl));

    std::shared_ptr<order_system::repository::IOrderRepository> repository;
    std::shared_ptr<shared::outbox::IOutboxRepository> outboxRepository;

    try {
        repository = std::make_shared<order_system::repository::PostgresOrderRepository>(databaseConfiguration);
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

    shared::outbox::OutboxPublisher outboxPublisher(outboxRepository, eventPublisher, "orders.created");
    outboxPublisher.start();

    std::shared_ptr<order_system::repository::PostgresOrderRepository> reservationRepository;
    try
    {
        reservationRepository = std::make_shared<order_system::repository::PostgresOrderRepository>(
            databaseConfiguration);
    }
    catch (const std::exception& e)
    {
        SPDLOG_LOGGER_CRITICAL(shared::logger::get("main"), "Error creating reservation repository: {}", e.what());
        return 1;
    }

    order_service::event_handlers::OrderReservedHandler reservedHandler(*reservationRepository, *reservationRepository);

    auto reservedConsumer = unwrapOrExit(shared::messaging::KafkaEventConsumer::createWithRetry(
        shared::messaging::KafkaConsumerConfiguration{
            .brokers = kafkaBrokers, .groupId = "order-service-group", .topic = "orders.reserved"
        },
        10, std::chrono::seconds(3)));

    if (auto startResult = reservedConsumer->start(reservedHandler); !startResult.has_value())
    {
        SPDLOG_LOGGER_CRITICAL(shared::logger::get("main"), "Failed to start Kafka consumer: {}",
                               startResult.error().message());
        outboxPublisher.stop();
        return 1;
    }

    shared::http::HttpServer server{std::move(networkConfiguration), buildRoutes(repository)};
    server.run();

    reservedConsumer->stop();
    outboxPublisher.stop();

    return EXIT_SUCCESS;
}

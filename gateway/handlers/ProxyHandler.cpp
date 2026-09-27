//
// Created by DED on 03.09.2026.
//

#include "ProxyHandler.h"

#include <http/HttpMessageConverter.h>
#include <logging/Logger.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core/flat_buffer.hpp>

namespace gateway_service::handlers {
    namespace asio = boost::asio;
    namespace beast = boost::beast;
    namespace http = beast::http;
    using tcp = boost::asio::ip::tcp;
    using namespace shared::models;

    ProxyHandler::ProxyHandler(
        std::unordered_map<std::string, UpstreamConfiguration> services) :
        _servicesUrl(std::move(services)) {
        // Заранее создаём по одному пулу на каждый известный апстрим, чтобы
        // в рантайме никогда не вставлять новые ключи в _pools конкурентно.
        for (const auto& pathPrefix : _servicesUrl | std::views::keys) {
            _pools[pathPrefix];
        }
    }

    std::optional<tcp::socket> ProxyHandler::acquirePooledSocket(const std::string& pathPrefix) {
        auto& pool = _pools.at(pathPrefix);

        std::scoped_lock lock(pool.mutex);
        if (pool.sockets.empty())
            return std::nullopt;

        auto socket = std::move(pool.sockets.back());
        pool.sockets.pop_back();
        return socket;
    }

    void ProxyHandler::releaseSocketIfReusable(
        const std::string& pathPrefix, tcp::socket socket,
        const http::response<http::string_body>& response) {

        if (!response.keep_alive() || !socket.is_open())
            return;

        auto& pool = _pools.at(pathPrefix);
        std::scoped_lock lock(pool.mutex);
        pool.sockets.push_back(std::move(socket));
    }

    asio::awaitable<tcp::socket> ProxyHandler::openSocket(
        const UpstreamConfiguration& config, asio::any_io_executor executor) {

        tcp::resolver resolver(executor);
        tcp::socket socket(executor);

        const auto& host = config.host.value();
        const auto port = std::to_string(config.port);

        const auto endpoints = co_await resolver.async_resolve(host, port, asio::use_awaitable);
        co_await asio::async_connect(socket, endpoints, asio::use_awaitable);

        co_return socket;
    }

    asio::awaitable<http::response<http::string_body>> ProxyHandler::exchange(
        tcp::socket& socket, const Request& request, const std::string& host) {

        auto beastRequest = shared::http::HttpMessageConverter::toBeastRequest(request);
        beastRequest.set(http::field::host, host);
        beastRequest.keep_alive(true);

        co_await http::async_write(socket, beastRequest, asio::use_awaitable);

        beast::flat_buffer buffer;
        http::response<http::string_body> beastResponse;
        co_await http::async_read(socket, buffer, beastResponse, asio::use_awaitable);

        co_return beastResponse;
    }

    asio::awaitable<Response> ProxyHandler::handle(const Request& request) {
        if (request.method == Method::Unknown) {
            co_return Response{.status = Status::MethodNotAllowed};
        }

        for (const auto& [pathPrefix, config] : _servicesUrl) {
            if (!request.path.starts_with(pathPrefix))
                continue;

            auto executor = co_await asio::this_coro::executor;
            const auto& host = config.host.value();

            // До двух попыток: первая - сокет из пула (если есть) или новый;
            // вторая (только если первая использовала сокет из пула и упала) -
            // гарантированно свежее соединение.
            for (int attempt = 0; attempt < 2; ++attempt) {
                const bool forceFresh = (attempt == 1);

                std::optional<tcp::socket> socket;
                bool usedPooledSocket = false;
                bool openFailed = false;

                if (!forceFresh) {
                    socket = acquirePooledSocket(pathPrefix);
                    usedPooledSocket = socket.has_value();
                }

                if (!socket) {
                    try {
                        socket = co_await openSocket(config, executor);
                    } catch (const boost::system::system_error& e) {
                        SPDLOG_LOGGER_WARN(shared::logger::get("ProxyHandler"),
                                           "Failed to open connection to upstream '{}': {}", pathPrefix, e.what());
                        openFailed = true;
                    }
                }

                if (openFailed) {
                    co_return Response{.status = Status::BadGateway};
                }

                bool exchangeFailed = false;
                std::string exchangeError;
                std::optional<http::response<http::string_body>> beastResponse;

                try {
                    beastResponse = co_await exchange(*socket, request, host);
                } catch (const boost::system::system_error& e) {
                    exchangeFailed = true;
                    exchangeError = e.what();
                }

                if (!exchangeFailed) {
                    releaseSocketIfReusable(pathPrefix, std::move(*socket), *beastResponse);
                    co_return shared::http::HttpMessageConverter::toHttpResponse(*beastResponse);
                }

                if (!usedPooledSocket) {
                    // Свежее соединение упало само по себе - повторять
                    // бессмысленно
                    SPDLOG_LOGGER_WARN(shared::logger::get("ProxyHandler"),
                                       "Proxy exchange with '{}' failed on a fresh connection: {}",
                                       pathPrefix, exchangeError);
                    co_return Response{.status = Status::BadGateway};
                }

                // Сокет был из пула - вероятно, апстрим успел закрыть его по
                // своему таймауту простоя.
                SPDLOG_LOGGER_DEBUG(shared::logger::get("ProxyHandler"),
                                    "Pooled connection to '{}' was stale ({}), retrying on a fresh one",
                                    pathPrefix, exchangeError);
            }

            co_return Response{.status = Status::BadGateway};
        }

        co_return Response{.status = Status::NotFound};
    }
};

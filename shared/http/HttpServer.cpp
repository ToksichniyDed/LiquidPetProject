//
// Created by DED on 22.08.2026.
//

#include "HttpServer.h"

#include <logging/Logger.h>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <chrono>
#include <functional>

#include "HttpMessageConverter.h"

namespace shared::http {
namespace beast = boost::beast;
namespace beast_http = beast::http;
namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace {
// Сколько соединение может простаивать между запросами (и сколько отводится на запись ответа)
constexpr auto CONNECTION_TIMEOUT = std::chrono::seconds(30);
} // namespace

HttpServer::HttpServer(models::NetworkConfiguration config, std::vector<handlers::Route> handlers,
                       std::size_t threadCount)
    : _networkConfiguration(std::move(config)),
      _ioContext(static_cast<int>(threadCount == 0 ? 1 : threadCount)),
      _acceptor(_ioContext, tcp::endpoint(boost::asio::ip::make_address(_networkConfiguration.address.value()),
                                          _networkConfiguration.port)),
      _signals(_ioContext, SIGINT, SIGTERM),
      _threadCount(threadCount == 0 ? 1 : threadCount),
      _handlers(std::move(handlers)) {
    _signals.async_wait([this](const boost::system::error_code& ec, int signalNumber) {
        if (ec)
            return;

        SPDLOG_LOGGER_INFO(shared::logger::get("HttpServer"), "Received signal {}, shutting down", signalNumber);
        stop();
    });

    SPDLOG_LOGGER_INFO(shared::logger::get("HttpServer"), "Server created successfully!");
    SPDLOG_LOGGER_INFO(shared::logger::get("HttpServer"), "Server address : {}", _networkConfiguration.address.value());
    SPDLOG_LOGGER_INFO(shared::logger::get("HttpServer"), "Server port : {}", _networkConfiguration.port);
}

HttpServer::~HttpServer() { SPDLOG_LOGGER_INFO(shared::logger::get("HttpServer"), "Server destroyed successfully!"); }

void HttpServer::run() {
    SPDLOG_LOGGER_INFO(shared::logger::get("HttpServer"), "Server run successfully!");

    doAccept();
    // Каждый поток вызывает run() на одном и том же io_context - это и даёт
    // многопоточную обработку: несколько корутин могут выполняться параллельно,
    // каждая на своём потоке, а io_context сам решает, кому что отдать.
    std::vector<std::thread> workers;
    for (std::size_t i = 1; i < _threadCount; ++i) {
        workers.emplace_back([this] { _ioContext.run(); });
    }

    _ioContext.run();

    for (auto& worker : workers) {
        worker.join();
    }
}

void HttpServer::stop() {
    SPDLOG_LOGGER_INFO(shared::logger::get("HttpServer"), "Server stop requested!");

    boost::system::error_code ec;
    _acceptor.close(ec);
    _ioContext.stop();
}

void HttpServer::doAccept() {
    _acceptor.async_accept([this](const beast::error_code& ec, tcp::socket socket) {
        if (!ec) {
            // co_spawn запускает handleSession как независимую корутину.
            // detached значит "не жду результата в этой точке" - сессия сама
            // себя обслужит до конца (или до ошибки), а этот код уже пошёл дальше.
            asio::co_spawn(_ioContext, handleSession(std::move(socket)), asio::detached);
        }

        if (_acceptor.is_open()) {
            doAccept();
        }
    });
}

asio::awaitable<void> HttpServer::handleSession(tcp::socket socket) const {
    beast::tcp_stream stream(std::move(socket));
    beast::flat_buffer buffer;

    try {
        while (true) {
            beast_http::request<beast_http::string_body> request;

            stream.expires_after(CONNECTION_TIMEOUT);
            co_await beast_http::async_read(stream, buffer, request, asio::use_awaitable);

            auto response = co_await handleRequest(request);
            response.version(request.version());
            response.keep_alive(request.keep_alive());

            stream.expires_after(CONNECTION_TIMEOUT);
            co_await beast_http::async_write(stream, response, asio::use_awaitable);

            if (!response.keep_alive())
                break;
        }
    } catch (const beast::system_error& e) {
        // end_of_stream - клиент штатно закрыл соединение
        if (e.code() != beast_http::error::end_of_stream) {
            SPDLOG_LOGGER_WARN(shared::logger::get("HttpServer"), "Session error: {}", e.what());
        }
    } catch (const std::exception& e) {
        SPDLOG_LOGGER_WARN(shared::logger::get("HttpServer"), "Session error: {}", e.what());
    }

    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_send, ec);
}

asio::awaitable<beast_http::response<beast_http::string_body>> HttpServer::handleRequest(
    const beast_http::request<beast_http::string_body>& beastRequest) const {
    const auto request = HttpMessageConverter::toHttpRequest(beastRequest);
    const auto handler = findHandler(request.method, request.path);

    models::Response response;
    if (!handler) {
        response.status = models::Status::NotFound;
        co_return HttpMessageConverter::toBeastResponse(response);
    }

    try {
        response = co_await handler->handle(request);
    } catch (const std::exception& e) {
        SPDLOG_LOGGER_ERROR(shared::logger::get("HttpServer"), "Unhandled exception in handler for {}: {}",
                            request.path, e.what());
        response = {.status = models::Status::InternalServerError, .body = "internal server error"};
    } catch (...) {
        SPDLOG_LOGGER_ERROR(shared::logger::get("HttpServer"), "Unhandled unknown exception in handler for {}",
                            request.path);
        response = {.status = models::Status::InternalServerError, .body = "internal server error"};
    }

    co_return HttpMessageConverter::toBeastResponse(response);
}

std::shared_ptr<IRequestHandler> HttpServer::findHandler(models::Method method, std::string_view path) const {
    for (const auto& [srcMethod, srcPathPrefix, srcHandler] : _handlers) {
        if (srcMethod == method && path.starts_with(srcPathPrefix)) return srcHandler;
    }
    return nullptr;
}
} // namespace shared::http

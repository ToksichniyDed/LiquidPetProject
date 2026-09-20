//
// Created by DED on 22.08.2026.
//

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>

#include <chrono>
#include <functional>

#include "HttpMessageConverter.h"
#include "HttpServer.h"
#include <logging/Logger.h>

namespace shared::http {

    namespace beast = boost::beast;
    namespace beast_http = beast::http;
    using tcp = boost::asio::ip::tcp;

    namespace {

        // Сколько соединение может простаивать между запросами (и сколько отводится на запись ответа)
        constexpr auto CONNECTION_TIMEOUT = std::chrono::seconds(30);

        using BeastRequest = beast_http::request<beast_http::string_body>;
        using BeastResponse = beast_http::response<beast_http::string_body>;
        using RequestHandler = std::function<BeastResponse(const BeastRequest&)>;

        // Одно клиентское соединение. Обслуживает запросы по очереди,
        // пока клиент не закроет соединение, не попросит "Connection: close" или не истечёт таймаут простоя.
        // Живёт, пока на неё ссылается хотя бы одна незавершённая асинхронная операция.
        class HttpSession : public std::enable_shared_from_this<HttpSession> {
        public:
            HttpSession(tcp::socket socket, RequestHandler handler)
                : _stream(std::move(socket)), _handler(std::move(handler)) {
            }

            void run() {
                doRead();
            }

        private:
            void doRead() {
                _request = {};
                _stream.expires_after(CONNECTION_TIMEOUT);

                beast_http::async_read(_stream, _buffer, _request,
                                       [self = shared_from_this()](const beast::error_code& ec, std::size_t) {
                                           self->onRead(ec);
                                       });
            }

            void onRead(const beast::error_code& ec) {
                if (ec == beast_http::error::end_of_stream)
                    return close();

                if (ec)
                    return;

                _response = _handler(_request);
                // Ответ повторяет версию и keep-alive запроса
                _response.version(_request.version());
                _response.keep_alive(_request.keep_alive());

                _stream.expires_after(CONNECTION_TIMEOUT);

                beast_http::async_write(_stream, _response,
                                        [self = shared_from_this()](const beast::error_code& writeEc, std::size_t) {
                                            self->onWrite(writeEc);
                                        });
            }

            void onWrite(const beast::error_code& ec) {
                if (ec)
                    return;

                if (!_response.keep_alive())
                    return close();

                doRead();
            }

            void close() {
                beast::error_code ec;
                _stream.socket().shutdown(tcp::socket::shutdown_send, ec);
            }

        private:
            beast::tcp_stream _stream;
            beast::flat_buffer _buffer;
            BeastRequest _request;
            BeastResponse _response;
            RequestHandler _handler;
        };

    }

    HttpServer::HttpServer(
        models::NetworkConfiguration config,
        std::vector<handlers::Route> handlers) : _networkConfiguration(std::move(config)),
                                                 _ioContext(1),
                                                 _acceptor(_ioContext, tcp::endpoint(
                                                               boost::asio::ip::make_address(
                                                                   _networkConfiguration.address.
                                                                   value()),
                                                               _networkConfiguration.port)),
                                                 _signals(_ioContext, SIGINT, SIGTERM),
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

    HttpServer::~HttpServer() {
        SPDLOG_LOGGER_INFO(shared::logger::get("HttpServer"), "Server destroyed successfully!");

    }

    void HttpServer::run() {
        SPDLOG_LOGGER_INFO(shared::logger::get("HttpServer"), "Server run successfully!");

        doAccept();
        _ioContext.run();
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
                std::make_shared<HttpSession>(std::move(socket), [this](const BeastRequest& request) {
                    return handleRequest(request);
                })->run();
            }

            if (_acceptor.is_open()) {
                doAccept();
            }
        });
    }

    beast_http::response<beast_http::string_body> HttpServer::handleRequest(
        const beast_http::request<beast_http::string_body>& beastRequest) const {

        const auto request = HttpMessageConverter::toHttpRequest(beastRequest);
        const auto handler = findHandler(request.method, request.path);

        models::Response response;
        if (!handler) {
            response.status = models::Status::NotFound;
            return HttpMessageConverter::toBeastResponse(response);
        }

        response = handler->handle(request);

        return HttpMessageConverter::toBeastResponse(response);
    }

    std::shared_ptr<IRequestHandler> HttpServer::findHandler(models::Method method, std::string_view path) const {
        for (const auto& [srcMethod, srcPathPrefix, srcHandler] : _handlers) {
            if (srcMethod == method && path.starts_with(srcPathPrefix))
                return srcHandler;
        }
        return nullptr;
    }
}

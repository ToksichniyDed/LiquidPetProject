//
// Created by DED on 22.08.2026.
//

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>

#include "HttpMessageConverter.h"
#include "HttpServer.h"
#include <logging/Logger.h>

namespace shared::http {

    namespace beast = boost::beast;
    namespace beast_http = beast::http;
    using tcp = boost::asio::ip::tcp;

    HttpServer::HttpServer(
        models::NetworkConfiguration config,
        std::vector<handlers::Route> handlers) : _networkConfiguration(std::move(config)),
                                                 _ioContext(1),
                                                 _acceptor(_ioContext, tcp::endpoint(
                                                               boost::asio::ip::make_address(
                                                                   _networkConfiguration.address.
                                                                   value()),
                                                               _networkConfiguration.port)),
                                                 _handlers(std::move(handlers)) {

        SPDLOG_LOGGER_INFO(Logger::get("HttpServer"), "Server created successfully!");
        SPDLOG_LOGGER_INFO(Logger::get("HttpServer"), "Server address : {}", _networkConfiguration.address.value());
        SPDLOG_LOGGER_INFO(Logger::get("HttpServer"), "Server port : {}", _networkConfiguration.port);
    }

    void HttpServer::run() {
        SPDLOG_LOGGER_INFO(Logger::get("HttpServer"), "Server run successfully!");

        doAccept();
        _ioContext.run();
    }

    void HttpServer::stop() {
        SPDLOG_LOGGER_INFO(Logger::get("HttpServer"), "Server stop requested!");

        boost::system::error_code ec;
        _acceptor.close(ec);
        _ioContext.stop();
    }

    void HttpServer::doAccept() {
        auto socket = std::make_shared<tcp::socket>(_ioContext);

        _acceptor.async_accept(*socket, [this, socket](const beast::error_code& ec) {
            if (!ec) {
                handleConnection(socket);
            }

            if (_acceptor.is_open()) {
                doAccept();
            }
        });
    }

    void HttpServer::handleConnection(std::shared_ptr<tcp::socket> socket) const {
        auto buffer = std::make_shared<beast::flat_buffer>();
        auto request = std::make_shared<beast_http::request<beast_http::string_body>>();

        beast_http::async_read(*socket, *buffer, *request,
                               [this, socket, buffer, request](const beast::error_code& ec, std::size_t) {
                                   if (ec)
                                       return;

                                   auto response = std::make_shared<beast_http::response<beast_http::string_body>>(
                                       handleRequest(*request));

                                   beast_http::async_write(*socket, *response,
                                                           [socket, response](const beast::error_code&, std::size_t) {
                                                               boost::system::error_code shutdownEc;
                                                               socket->shutdown(tcp::socket::shutdown_send, shutdownEc);
                                                           });
                               });
    }

    beast_http::response<beast_http::string_body> HttpServer::handleRequest(
        const beast_http::request<beast_http::string_body>& beastRequest) const {

        const auto request = HttpMessageConverter::toHttpRequest(beastRequest);
        const auto handler = findHandler(request.method, request.path);

        Response response;
        if (!handler) {
            response.status = Status::NotFound;
            return HttpMessageConverter::toBeastResponse(response);
        }

        response = handler->handle(request);

        return HttpMessageConverter::toBeastResponse(response);
    }

    std::shared_ptr<IRequestHandler> HttpServer::findHandler(Method method, std::string_view path) const {
        for (const auto& [srcMethod, srcPathPrefix, srcHandler] : _handlers) {
            if (srcMethod == method && path.starts_with(srcPathPrefix))
                return srcHandler;
        }
        return nullptr;
    }
}

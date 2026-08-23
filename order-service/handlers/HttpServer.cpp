//
// Created by DED on 22.08.2026.
//

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>

#include "HttpServer.h"

namespace order_service::handlers {

    namespace beast = boost::beast;
    namespace http = beast::http;
    using tcp = boost::asio::ip::tcp;

    HttpServer::HttpServer(order_system::models::NetworkConfiguration config): _networkConfiguration(std::move(config)), _ioContext(1), _acceptor(_ioContext, tcp::endpoint(
                                                                                                                                                      boost::asio::ip::make_address(_networkConfiguration.address.value()), _networkConfiguration.port))  {
    }

    void HttpServer::run() {
        accept();
    }

    void HttpServer::accept() {
        while (true) {
            tcp::socket socket(_ioContext);
            _acceptor.accept(socket);
            handleConnection(std::move(socket));
        }
    }

    void HttpServer::handleConnection(boost::asio::ip::tcp::socket socket) const {
        beast::error_code ec;
        beast::flat_buffer buffer;
        http::request<http::string_body> request;
        http::read(socket, buffer, request, ec);

        if (ec) {
            return;
        }

        http::response<http::string_body> response{http::status::ok, request.version()};
        response.set(http::field::server, "order-service");
        response.set(http::field::content_type, "text/plain");
        response.body() = "OK";
        response.prepare_payload();

        http::write(socket, response, ec);
        socket.shutdown(tcp::socket::shutdown_send, ec);
    }
}

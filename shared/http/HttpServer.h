//
// Created by DED on 22.08.2026.
//

#ifndef LIQUIDPETPROJECT_HTTPSERVER_H
#define LIQUIDPETPROJECT_HTTPSERVER_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "NetworkConfiguration.h"
#include "Route.h"

namespace order_service::handlers {
    using namespace boost::beast;

    class HttpServer {
        public:
        explicit HttpServer(order_system::models::NetworkConfiguration config, std::vector<Route> handlers);

        [[noreturn]] void run();

    private:
        [[noreturn]] void accept();
        void handleConnection(boost::asio::ip::tcp::socket socket) const;
        http::response<http::string_body> handleRequest(const http::request<http::string_body>& beastRequest) const;
        std::shared_ptr<IRequestHandler> findHandler(Http::Method method, std::string_view path) const;

    private:
        order_system::models::NetworkConfiguration _networkConfiguration;
        boost::asio::io_context _ioContext;
        boost::asio::ip::tcp::acceptor _acceptor;

        std::vector<Route> _handlers;
    };

}

#endif //LIQUIDPETPROJECT_HTTPSERVER_H

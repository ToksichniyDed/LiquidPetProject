//
// Created by DED on 22.08.2026.
//

#ifndef LIQUIDPETPROJECT_HTTPSERVER_H
#define LIQUIDPETPROJECT_HTTPSERVER_H

#include <boost/asio.hpp>

#include <NetworkConfiguration.h>

namespace order_service::handlers {
    class HttpServer {
        explicit HttpServer(order_system::models::NetworkConfiguration config);

        [[noreturn]] void run();

    private:
        [[noreturn]] void accept();
        void handleConnection(boost::asio::ip::tcp::socket socket) const;

    private:
        order_system::models::NetworkConfiguration _networkConfiguration;
        boost::asio::io_context _ioContext;
        boost::asio::ip::tcp::acceptor _acceptor;
    };

}

#endif //LIQUIDPETPROJECT_HTTPSERVER_H

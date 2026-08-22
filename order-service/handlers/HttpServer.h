//
// Created by DED on 22.08.2026.
//

#ifndef LIQUIDPETPROJECT_HTTPSERVER_H
#define LIQUIDPETPROJECT_HTTPSERVER_H

#include <boost/asio.hpp>

#include <NetworkAddress.h>

namespace order_service::handlers {

    struct NetworkConfiguration {
        order_system::models::NetworkAddress address;
        uint16_t port{8080};
    };

    class HttpServer {
        explicit HttpServer(NetworkConfiguration config);

        [[noreturn]] void run();

    private:
        [[noreturn]] void accept();
        void handleConnection(boost::asio::ip::tcp::socket socket) const;

    private:
        NetworkConfiguration _networkConfiguration;
        boost::asio::io_context _ioContext;
        boost::asio::ip::tcp::acceptor _acceptor;
    };

}

#endif //LIQUIDPETPROJECT_HTTPSERVER_H

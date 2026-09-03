//
// Created by DED on 22.08.2026.
//

#ifndef LIQUIDPETPROJECT_HTTPSERVER_H
#define LIQUIDPETPROJECT_HTTPSERVER_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "NetworkConfiguration.h"
#include "Route.h"

namespace shared::http {
    class HttpServer {
        public:
        explicit HttpServer(models::NetworkConfiguration config, std::vector<handlers::Route> handlers);

        void run();
        void stop();

    private:
        void doAccept();
        void handleConnection(std::shared_ptr<boost::asio::ip::tcp::socket> socket) const;
        boost::beast::http::response<boost::beast::http::string_body> handleRequest(
            const boost::beast::http::request<boost::beast::http::string_body>& beastRequest) const;
        std::shared_ptr<IRequestHandler> findHandler(Method method, std::string_view path) const;

    private:
        models::NetworkConfiguration _networkConfiguration;
        boost::asio::io_context _ioContext;
        boost::asio::ip::tcp::acceptor _acceptor;

        std::vector<handlers::Route> _handlers;
    };

}

#endif //LIQUIDPETPROJECT_HTTPSERVER_H

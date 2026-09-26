//
// Created by DED on 22.08.2026.
//

#ifndef LIQUIDPETPROJECT_HTTPSERVER_H
#define LIQUIDPETPROJECT_HTTPSERVER_H

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast.hpp>

#include <models/NetworkConfiguration.h>
#include "Route.h"

namespace shared::http {
    class HttpServer {
        public:
        explicit HttpServer(models::NetworkConfiguration config, std::vector<handlers::Route> handlers,
                              std::size_t threadCount = std::thread::hardware_concurrency());
        ~HttpServer();

        void run();
        void stop();

    private:
        void doAccept();
        boost::asio::awaitable<void> handleSession(boost::asio::ip::tcp::socket socket) const;
       boost::asio::awaitable<boost::beast::http::response<boost::beast::http::string_body>> handleRequest(
            const boost::beast::http::request<boost::beast::http::string_body>& beastRequest) const;
        std::shared_ptr<IRequestHandler> findHandler(models::Method method, std::string_view path) const;

    private:
        models::NetworkConfiguration _networkConfiguration;
        boost::asio::io_context _ioContext;
        boost::asio::ip::tcp::acceptor _acceptor;
        boost::asio::signal_set _signals;
        std::size_t _threadCount;

        std::vector<handlers::Route> _handlers;
    };

}

#endif //LIQUIDPETPROJECT_HTTPSERVER_H

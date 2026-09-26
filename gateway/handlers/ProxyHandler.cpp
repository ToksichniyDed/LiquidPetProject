//
// Created by DED on 03.09.2026.
//

#include "ProxyHandler.h"

#include <http/HttpMessageConverter.h>

#include <boost/beast/core/flat_buffer.hpp>

namespace gateway_service::handlers {
    namespace asio = boost::asio;
    using namespace shared::models;

    ProxyHandler::ProxyHandler(
        std::unordered_map<std::string, UpstreamConfiguration> services) :
        _servicesUrl(std::move(services)) {
    }

    boost::asio::awaitable<Response> ProxyHandler::handle(const Request& request) {
        if (request.method == Method::Unknown) {
            co_return Response{.status = Status::MethodNotAllowed};
        }

        for (const auto& [pathPrefix, config] : _servicesUrl) {
            if (!request.path.starts_with(pathPrefix))
                continue;

            namespace beast = boost::beast;
            namespace http = beast::http;
            using tcp = boost::asio::ip::tcp;

            // Узнаём, на каком executor'е нас сейчас выполняет io_context -
            // это мгновенная операция, без реальной заморозки корутины.
            auto executor = co_await asio::this_coro::executor;

            tcp::resolver resolver(executor);
            tcp::socket socket(executor);

            const auto& host = config.host.value();
            const auto port = std::to_string(config.port);

            try {
                // Заморозка: ждём резолвинг адреса (может быть DNS-запрос)
                const auto endpoints = co_await resolver.async_resolve(host, port, asio::use_awaitable);

                // Заморозка: ждём установления TCP-соединения
                co_await asio::async_connect(socket, endpoints, asio::use_awaitable);

                auto beastRequest = shared::http::HttpMessageConverter::toBeastRequest(request);
                beastRequest.set(http::field::host, host);

                // Заморозка: ждём, пока весь запрос уйдёт в сокет
                co_await http::async_write(socket, beastRequest, asio::use_awaitable);

                beast::flat_buffer buffer;
                http::response<http::string_body> beastResponse;

                // Заморозка: ждём ответ от апстрима
                co_await http::async_read(socket, buffer, beastResponse, asio::use_awaitable);

                boost::system::error_code ec;
                socket.shutdown(tcp::socket::shutdown_both, ec);

                co_return shared::http::HttpMessageConverter::toHttpResponse(beastResponse);

            } catch (const boost::system::system_error&) {
                // resolve/connect/write/read - любая из этих операций могла
                // упасть с ошибкой сети. Всё это сейчас трактуем как BadGateway
                co_return Response{.status = Status::BadGateway};
            }
        }

        co_return Response{.status = Status::NotFound};
    }
};

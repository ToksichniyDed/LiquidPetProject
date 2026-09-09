//
// Created by DED on 03.09.2026.
//

#include "ProxyHandler.h"

#include <http/HttpMessageConverter.h>

#include <boost/beast/core/flat_buffer.hpp>

namespace gateway_service::handlers {
    using namespace shared::models;

    ProxyHandler::ProxyHandler(
        std::unordered_map<std::string, NetworkConfiguration> services) : _ioContext(1),
        _servicesUrl(std::move(services)) {
    }

    Response ProxyHandler::handle(const Request& request) {
        if (request.method == Method::Unknown) {
            return {.status = Status::MethodNotAllowed};
        }

        for (const auto& [pathPrefix, config] : _servicesUrl) {
            if (request.path.starts_with(pathPrefix)) {
                namespace beast = boost::beast;
                namespace http = beast::http;
                using tcp = boost::asio::ip::tcp;

                tcp::resolver resolver(_ioContext);
                tcp::socket socket(_ioContext);

                const auto& host = config.address.value();
                const auto port = std::to_string(config.port);

                beast::error_code ec;
                const auto endpoints = resolver.resolve(host, port, ec);
                if (ec) {
                    return {.status = Status::BadGateway};
                }

                boost::asio::connect(socket, endpoints, ec);
                if (ec) {
                    return {.status = Status::BadGateway};
                }

                auto beastRequest = shared::http::HttpMessageConverter::toBeastRequest(request);
                beastRequest.set(http::field::host, host);

                http::write(socket, beastRequest, ec);
                if (ec) {
                    return {.status = Status::BadGateway};
                }

                beast::flat_buffer buffer;
                http::response<http::string_body> beastResponse;
                http::read(socket, buffer, beastResponse, ec);
                if (ec) {
                    return {.status = Status::BadGateway};
                }

                socket.shutdown(tcp::socket::shutdown_both, ec);

                return shared::http::HttpMessageConverter::toHttpResponse(beastResponse);
            }
        }

        return {.status = Status::NotFound};
    }
};

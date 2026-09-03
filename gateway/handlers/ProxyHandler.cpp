//
// Created by DED on 03.09.2026.
//

#include "ProxyHandler.h"
#include <http/HttpMessageConverter.h>

namespace gateway_service::handlers {

    ProxyHandler::ProxyHandler(
        std::unordered_map<std::string, shared::http::models::NetworkConfiguration> services) : _ioContext(1),
        _servicesUrl(std::move(services)) {
    }

    shared::http::Response ProxyHandler::handle(const shared::http::Request& request) {
        if (request.method == shared::http::Method::Unknown) {
            return {.status = shared::http::Status::MethodNotAllowed};
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
                    return {.status = shared::http::Status::BadGateway};
                }

                boost::asio::connect(socket, endpoints, ec);
                if (ec) {
                    return {.status = shared::http::Status::BadGateway};
                }

                auto beastRequest = shared::http::HttpMessageConverter::toBeastRequest(request);
                beastRequest.set(http::field::host, host);

                http::write(socket, beastRequest, ec);
                if (ec) {
                    return {.status = shared::http::Status::BadGateway};
                }

                beast::flat_buffer buffer;
                http::response<http::string_body> beastResponse;
                http::read(socket, buffer, beastResponse, ec);
                if (ec) {
                    return {.status = shared::http::Status::BadGateway};
                }

                socket.shutdown(tcp::socket::shutdown_both, ec);

                return shared::http::HttpMessageConverter::toHttpResponse(beastResponse);
            }
        }

        return {.status = shared::http::Status::NotFound};
    }
};

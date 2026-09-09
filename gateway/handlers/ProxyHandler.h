//
// Created by DED on 03.09.2026.
//

#ifndef LIQUIDPETPROJECT_PROXYHANDLER_H
#define LIQUIDPETPROJECT_PROXYHANDLER_H

#include <models/NetworkConfiguration.h>
#include <http/IRequestHandler.h>

#include <boost/asio.hpp>
#include <unordered_map>

namespace gateway_service::handlers {

    class ProxyHandler : public shared::http::IRequestHandler {
    public:
        explicit ProxyHandler(std::unordered_map<std::string, shared::models::NetworkConfiguration> services);
        ~ProxyHandler() override = default;
        shared::models::Response handle(const shared::models::Request& request) override;

    private:
        boost::asio::io_context _ioContext;

        std::unordered_map<std::string, shared::models::NetworkConfiguration> _servicesUrl;

    };
}

#endif //LIQUIDPETPROJECT_PROXYHANDLER_H

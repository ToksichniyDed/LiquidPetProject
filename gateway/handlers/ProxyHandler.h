//
// Created by DED on 03.09.2026.
//

#ifndef LIQUIDPETPROJECT_PROXYHANDLER_H
#define LIQUIDPETPROJECT_PROXYHANDLER_H

#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <http/IRequestHandler.h>
#include <http/NetworkAddress.h>
#include <http/NetworkConfiguration.h>

namespace gateway_service::handlers {

    class ProxyHandler : public shared::http::IRequestHandler {
    public:
        explicit ProxyHandler(std::unordered_map<std::string, shared::http::models::NetworkConfiguration> services);
        ~ProxyHandler() override = default;
        shared::http::Response handle(const shared::http::Request& request) override;

    private:
        boost::asio::io_context _ioContext;

        std::unordered_map<std::string, shared::http::models::NetworkConfiguration> _servicesUrl;

    };
}

#endif //LIQUIDPETPROJECT_PROXYHANDLER_H

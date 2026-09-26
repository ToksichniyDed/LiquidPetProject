//
// Created by DED on 17.09.2026.
//

#ifndef LIQUIDPETPROJECT_GATEWAY_HEALTHHANDLER_H
#define LIQUIDPETPROJECT_GATEWAY_HEALTHHANDLER_H

#include <http/IRequestHandler.h>

namespace gateway_service::handlers {
class HealthHandler : public shared::http::IRequestHandler {
public:
    boost::asio::awaitable<shared::models::Response> handle(const shared::models::Request& /*request*/) override {
        co_return shared::models::Response{
            .status = shared::models::Status::Ok,
            .body = "OK"
        };
    }
};
}

#endif //LIQUIDPETPROJECT_GATEWAY_HEALTHHANDLER_H

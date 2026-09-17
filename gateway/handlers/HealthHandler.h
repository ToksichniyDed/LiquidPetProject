//
// Created by DED on 17.09.2026.
//

#ifndef LIQUIDPETPROJECT_GATEWAY_HEALTHHANDLER_H
#define LIQUIDPETPROJECT_GATEWAY_HEALTHHANDLER_H

#include <http/IRequestHandler.h>

namespace gateway_service::handlers {
class HealthHandler : public shared::http::IRequestHandler {
public:
    shared::models::Response handle(const shared::models::Request& /*request*/) override {
        return {
            .status = shared::models::Status::Ok,
            .body = "OK"
        };
    }
};
}

#endif //LIQUIDPETPROJECT_GATEWAY_HEALTHHANDLER_H

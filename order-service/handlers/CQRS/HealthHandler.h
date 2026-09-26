//
// Created by DED on 27.08.2026.
//

#ifndef LIQUIDPETPROJECT_HEALTHHANDLER_H
#define LIQUIDPETPROJECT_HEALTHHANDLER_H

#include <http/IRequestHandler.h>

namespace order_service::handlers {
    class HealthHandler : public IRequestHandler {
    public:
        boost::asio::awaitable<Response> handle(const Request& /*request*/) override {
            co_return Response{
                .status = Status::Ok,
                .body = "OK"
            };
        }
    };
}

#endif //LIQUIDPETPROJECT_HEALTHHANDLER_H

//
// Created by DED on 27.08.2026.
//

#ifndef LIQUIDPETPROJECT_CREATEORDERHANDLER_H
#define LIQUIDPETPROJECT_CREATEORDERHANDLER_H

#include <http/IRequestHandler.h>
#include "IOrderRepository.h"
#include <mapper/OrderJsonMapper.h>

namespace order_service::handlers {
    using namespace order_system::models;
    using namespace order_system::repository;
    using namespace order_system::models2json_mapper;

    class CreateOrderHandler : public IRequestHandler {
    public:
        CreateOrderHandler(const std::shared_ptr<IOrderRepository>& orderRepository) : _orderRepository(
            orderRepository) {
        };

        Http::Response handle(const Http::Request& request) override {
            nlohmann::json bodyJson;
            try {
                bodyJson = nlohmann::json::parse(request.body);
            } catch (const nlohmann::json::parse_error&) {
                return {.status = Http::Status::BadRequest, .body = bodyJson.dump()};
            }

            auto orderResult = OrderJsonMapper::fromJson(bodyJson);
            if (!orderResult.has_value())
                return {.status = Http::Status::BadRequest, .body = orderResult.error().message()};

            Order order = std::move(orderResult.value());

            auto saveResult = _orderRepository->save(order);

            if (!saveResult.has_value())
                return {.status = Http::Status::InternalServerError, .body = orderResult.error().message()};

            return {.status = Http::Status::Ok, .body = {}};
        };

    private:
        std::shared_ptr<IOrderRepository> _orderRepository;
    };
}

#endif //LIQUIDPETPROJECT_CREATEORDERHANDLER_H

//
// Created by DED on 27.08.2026.
//

#ifndef LIQUIDPETPROJECT_CREATEORDERHANDLER_H
#define LIQUIDPETPROJECT_CREATEORDERHANDLER_H

#include <http/IRequestHandler.h>
#include "IOrderRepository.h"
#include <mapper/OrderJsonMapper.h>

namespace order_service::handlers {
    using namespace shared::http;
    using namespace shared::models;
    using namespace order_system::models;
    using namespace order_system::repository;
    using namespace order_system::models2json_mapper;

    class CreateOrderHandler : public IRequestHandler {
    public:
        CreateOrderHandler(const std::shared_ptr<IOrderRepository>& orderRepository) : _orderRepository(
            orderRepository) {
        };

        boost::asio::awaitable<Response> handle(const Request& request) override {
            nlohmann::json bodyJson;
            try {
                bodyJson = nlohmann::json::parse(request.body);
            } catch (const nlohmann::json::parse_error&) {
                co_return Response{.status = Status::BadRequest, .body = bodyJson.dump()};
            }

            auto orderResult = OrderJsonMapper::fromJson(bodyJson);
            if (!orderResult.has_value())
                co_return Response{.status = Status::BadRequest, .body = orderResult.error().message()};

            Order order = std::move(orderResult.value());

            auto saveResult = _orderRepository->save(order);

            if (!saveResult.has_value())
                co_return Response{.status = Status::InternalServerError, .body = saveResult.error().message()};

            nlohmann::json responseJson;
            responseJson[ORDER_ID] = saveResult.value().value();
            co_return Response{.status = Status::Created, .body = responseJson.dump()};
        };

    private:
        std::shared_ptr<IOrderRepository> _orderRepository;
    };
}

#endif //LIQUIDPETPROJECT_CREATEORDERHANDLER_H

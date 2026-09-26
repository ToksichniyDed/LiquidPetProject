//
// Created by DED on 27.08.2026.
//

#ifndef LIQUIDPETPROJECT_GETORDERHANDLER_H
#define LIQUIDPETPROJECT_GETORDERHANDLER_H

#include "RoutePaths.h"
#include <http/IRequestHandler.h>
#include "IOrderRepository.h"

namespace order_service::handlers {
    class GetOrderHandler : public IRequestHandler {
    public:
        GetOrderHandler(
            const std::shared_ptr<IOrderRepository>& orderRepository) : _orderRepository(
            orderRepository) {
        };

        boost::asio::awaitable<Response> handle(const Request& request) override {
            auto id = request.path.substr(paths::ORDERS_PREFIX.size());

            auto orderIdResult = OrderId::create(id);
            if (!orderIdResult.has_value()) {
                co_return Response{.status = Status::BadRequest, .body = orderIdResult.error().message()};
            }

            auto orderResult = _orderRepository->findById(orderIdResult.value());
            if (!orderResult) {
                if (orderResult.error() == RepositoryError::NotFound) {
                    co_return Response{
                        .status = Status::NotFound,
                        .body = orderResult.error().message()
                    };
                }

                co_return Response{
                    .status = Status::InternalServerError,
                    .body = orderResult.error().message()
                };
            }


            co_return Response{
                .status = Status::Ok,
                .body = OrderJsonMapper::toJson(orderResult.value()).dump()
            };
        };

    private:
        std::shared_ptr<IOrderRepository> _orderRepository;
    };
}

#endif //LIQUIDPETPROJECT_GETORDERHANDLER_H

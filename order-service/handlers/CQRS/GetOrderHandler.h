//
// Created by DED on 27.08.2026.
//

#ifndef LIQUIDPETPROJECT_GETORDERHANDLER_H
#define LIQUIDPETPROJECT_GETORDERHANDLER_H

#include <handlers/IRequestHandler.h>
#include <repository/IOrderRepository.h>
#include <handlers/RoutePaths.h>

namespace order_service::handlers {
    class GetOrderHandler : public IRequestHandler {
    public:
        GetOrderHandler(
            const std::shared_ptr<order_system::repository::IOrderRepository>& orderRepository) : _orderRepository(
            orderRepository) {
        };

        Http::Response handle(const Http::Request& request) override {
            auto id = request.path.substr(paths::ORDERS_PREFIX.size());

            auto orderIdResult = OrderId::create(id);
            if (!orderIdResult.has_value()) {
                return {.status = Http::Status::BadRequest, .body = orderIdResult.error().message()};
            }

            auto orderResult = _orderRepository->findById(orderIdResult.value());
            if (orderResult.error() == RepositoryError::NotFound) {
                return {
                    .status = Http::Status::NotFound,
                    .body = orderResult.error().message()
                };
            }
            return {
                .status = Http::Status::InternalServerError,
                .body = orderResult.error().message()
            };


            return {
                .status = Http::Status::Ok,
                .body = OrderJsonMapper::toJson(orderResult.value()).dump()
            };
        };

    private:
        std::shared_ptr<IOrderRepository> _orderRepository;
    };
}

#endif //LIQUIDPETPROJECT_GETORDERHANDLER_H

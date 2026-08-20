//
// Created by DED on 18.08.2026.
//

#ifndef LIQUIDPETPROJECT_IORDERREPOSITORY_H
#define LIQUIDPETPROJECT_IORDERREPOSITORY_H

#include <expected>
#include <system_error>

#include "Order.h"

namespace order_system::repository {
    enum class RepositoryError {
        NotFound = 1,
        ConnectionFailure,
        Timeout,
        ConstraintViolation,
        SerializationFailure
    };

    class RepositoryErrorCategory : public std::error_category {
        const char* name() const noexcept override { return "repository"; }

        std::string message(int ev) const override {
            switch (static_cast<RepositoryError>(ev)) {
                case RepositoryError::NotFound:
                    return "not found";
                case RepositoryError::ConnectionFailure:
                    return "connection failure";
                case RepositoryError::Timeout:
                    return "timeout";
                case RepositoryError::ConstraintViolation:
                    return "constraint violation failure";
                case RepositoryError::SerializationFailure:
                    return "serialization failure";
                default:
                    return "unknown repository error";
            }
        }
    };

    inline const RepositoryErrorCategory& repositoryErrorCategory() {
        static RepositoryErrorCategory instance;
        return instance;
    }

    inline std::error_code make_error_code(RepositoryError e) {
        return std::error_code(static_cast<int>(e), repositoryErrorCategory());
    }
}

namespace std {
    template <>
    struct is_error_code_enum<order_system::repository::RepositoryError> : true_type {
    };
}

namespace order_system::repository {
    class IOrderRepository {
    public:
        IOrderRepository() = default;
        virtual ~IOrderRepository() = default;

        [[nodiscard]] virtual std::expected<models::OrderId, std::error_code> save(
            const models::Order& order) = 0;
        [[nodiscard]] virtual std::expected<models::Order, std::error_code> findById(
            const models::OrderId& id) = 0;
    };
}

#endif //LIQUIDPETPROJECT_IORDERREPOSITORY_H

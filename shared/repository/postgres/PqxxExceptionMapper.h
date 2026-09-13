//
// Created by DED on 13.09.2026.
//

#ifndef LIQUIDPETPROJECT_PQXXEXCEPTIONMAPPER_H
#define LIQUIDPETPROJECT_PQXXEXCEPTIONMAPPER_H

#include <pqxx/pqxx>
#include <system_error>

#include "RepositoryError.h"

namespace shared::repository::postgres {

inline std::error_code mapPqxxException(const std::exception& e) {
    using enum RepositoryError;

    if (dynamic_cast<const pqxx::broken_connection*>(&e)) return make_error_code(ConnectionFailure);

    if (dynamic_cast<const pqxx::unique_violation*>(&e) || dynamic_cast<const pqxx::foreign_key_violation*>(&e) ||
        dynamic_cast<const pqxx::check_violation*>(&e))
        return make_error_code(ConstraintViolation);

    if (dynamic_cast<const pqxx::in_doubt_error*>(&e)) return make_error_code(Timeout);

    return make_error_code(SerializationFailure);
}

}  // namespace shared::repository::postgres

#endif  // LIQUIDPETPROJECT_PQXXEXCEPTIONMAPPER_H

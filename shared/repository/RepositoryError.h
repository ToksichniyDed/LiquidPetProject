//
// Created by DED on 13.09.2026.
//

#ifndef LIQUIDPETPROJECT_REPOSITORYERROR_H
#define LIQUIDPETPROJECT_REPOSITORYERROR_H

#include <string>
#include <system_error>

namespace shared::repository {

enum class RepositoryError : std::uint8_t {
    NotFound = 1,
    ConnectionFailure,
    Timeout,
    ConstraintViolation,
    SerializationFailure
};

class RepositoryErrorCategory : public std::error_category {
   public:
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

inline std::error_code make_error_code(RepositoryError e) { return {static_cast<int>(e), repositoryErrorCategory()}; }

}  // namespace shared::repository

namespace std {
template <>
struct is_error_code_enum<shared::repository::RepositoryError> : true_type {};
}  // namespace std

#endif  // LIQUIDPETPROJECT_REPOSITORYERROR_H

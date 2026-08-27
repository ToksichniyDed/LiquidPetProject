//
// Created by DED on 16.08.2026.
//

#ifndef LIQUIDPETPROJECT_STRONGID_H
#define LIQUIDPETPROJECT_STRONGID_H

#include <expected>
#include <regex>
#include <string>
#include <system_error>

namespace order_system::models {

    template <typename Tag>
    struct IdTraits;

    enum class StrongIdError : std::uint8_t {
        EmptyId = 1,
        InvalidFormat
    };

    template <typename Tag>
    class StrongID {
    public:
        static std::expected<StrongID, std::error_code> create(std::string value) {
            if (value.empty())
                return std::unexpected(StrongIdError::EmptyId);

            if (!IdTraits<Tag>::isValid(value))
                return std::unexpected(StrongIdError::InvalidFormat);

            return StrongID<Tag>{std::move(value)};
        }

        const std::string& value() const {
            return _value;
        }

        bool operator==(const StrongID& other) const = default;

    private:
        explicit StrongID(std::string value) : _value(std::move(value)) {
        }

    private:
        std::string _value;
    };

    class StrongIdErrorCategory : public std::error_category {
    public:
        const char* name() const noexcept override {
            return "strongId";
        }

        std::string message(int ev) const override {
            switch (static_cast<StrongIdError>(ev)) {
                case StrongIdError::EmptyId:
                    return "empty id";
                case StrongIdError::InvalidFormat:
                    return "invalid format";
                default:
                    return "unknown strongId error";
            }
        }
    };

    inline const StrongIdErrorCategory& strongIdErrorCategory() {
        static StrongIdErrorCategory instance;
        return instance;
    }

    inline std::error_code make_error_code(StrongIdError e) {
        return {static_cast<int>(e), strongIdErrorCategory()};
    }

}

namespace std {
    template <typename Tag>
    struct hash<order_system::models::StrongID<Tag>> {
        std::size_t operator()(const order_system::models::StrongID<Tag>& id) const noexcept {
            return std::hash<std::string>{}(id.value());
        }
    };

    template <>
    struct is_error_code_enum<order_system::models::StrongIdError> : true_type {
    };
}

#endif //LIQUIDPETPROJECT_STRONGID_H

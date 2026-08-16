//
// Created by DED on 16.08.2026.
//

#ifndef LIQUIDPETPROJECT_STRONGID_H
#define LIQUIDPETPROJECT_STRONGID_H

#include <expected>
#include <regex>
#include <string>

namespace order_system::models {

    template <typename Tag>
    struct IdTraits;

    template <typename Tag>
    class StrongID {
    public:

        enum class Error {
            EmptyId,
            InvalidFormat
        };
    public:

        static std::expected<StrongID, Error> create(std::string value) {
            if (value.empty())
                return std::unexpected(Error::EmptyId);

            if (!IdTraits<Tag>::isValid(value))
                return std::unexpected(Error::InvalidFormat);

            return StrongID<Tag>{std::move(value)};
        };
        const std::string& value() const {
            return _value;
        }

        bool operator==(const StrongID& other) const = default;

    private:
        explicit StrongID(std::string value) : _value(std::move(value)) {
        };

    private:
        std::string _value;
    };
}

namespace std {
    template <typename Tag>
    struct hash<order_system::models::StrongID<Tag>> {
        std::size_t operator()(const order_system::models::StrongID<Tag>& id) const noexcept {
            return std::hash<std::string>{}(id.value());
        }
    };
}

#endif //LIQUIDPETPROJECT_STRONGID_H

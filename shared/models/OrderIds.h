//
// Created by DED on 10.09.2026.
//

#ifndef LIQUIDPETPROJECT_ORDERIDS_H
#define LIQUIDPETPROJECT_ORDERIDS_H

#include <regex>
#include <string>

#include "StrongID.h"

namespace shared::models {

struct OrderIdTag {};
struct UserIdTag {};
struct ProductIdTag {};
struct OutboxEventIdTag {};

namespace detail {
inline bool isValidUuid(const std::string& value) {
    static const std::regex pattern(R"(^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$)");
    return std::regex_match(value, pattern);
}
}  // namespace detail

template <>
struct IdTraits<OrderIdTag> {
    static bool isValid(const std::string& v) { return detail::isValidUuid(v); }
};

template <>
struct IdTraits<UserIdTag> {
    static bool isValid(const std::string& v) { return detail::isValidUuid(v); }
};

template <>
struct IdTraits<ProductIdTag> {
    static bool isValid(const std::string& v) { return detail::isValidUuid(v); }
};

template <>
struct IdTraits<OutboxEventIdTag> {
    static bool isValid(const std::string& v) { return detail::isValidUuid(v); }
};

using OrderId = StrongID<OrderIdTag>;
using UserId = StrongID<UserIdTag>;
using ProductId = StrongID<ProductIdTag>;
using OutboxEventId = StrongID<OutboxEventIdTag>;

}  // namespace shared::models

#endif  // LIQUIDPETPROJECT_ORDERIDS_H

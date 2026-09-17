//
// Created by DED on 17.09.2026.
//

#ifndef LIQUIDPETPROJECT_HOST_H
#define LIQUIDPETPROJECT_HOST_H

#include <algorithm>
#include <cctype>
#include <expected>
#include <ranges>
#include <string>
#include <system_error>

#include <boost/asio/ip/address.hpp>

namespace shared::models {

    enum class HostError : std::uint8_t {
        EmptyHost = 1,
        InvalidFormat
    };

    class HostErrorCategory : public std::error_category {
    public:
        const char* name() const noexcept override { return "host"; }

        std::string message(int ev) const override {
            switch (static_cast<HostError>(ev)) {
                case HostError::EmptyHost:
                    return "empty host";
                case HostError::InvalidFormat:
                    return "invalid format";
                default:
                    return "unknown host error";
            }
        }
    };

    inline const HostErrorCategory& hostErrorCategory() {
        static HostErrorCategory instance;
        return instance;
    }

    inline std::error_code make_error_code(HostError e) {
        return {static_cast<int>(e), hostErrorCategory()};
    }

}

namespace std {
    template <>
    struct is_error_code_enum<shared::models::HostError> : true_type {
    };
}

namespace shared::models {

    namespace detail {

        inline bool isValidHostnameLabel(std::string_view label) {
            if (label.empty() || label.size() > 63)
                return false;

            if (label.front() == '-' || label.back() == '-')
                return false;

            return std::ranges::all_of(label, [](unsigned char c) {
                return std::isalnum(c) || c == '-';
            });
        }

        inline bool isValidHostname(const std::string& value) {
            if (value.empty() || value.size() > 253)
                return false;

            for (const auto labelRange : value | std::views::split('.')) {
                if (!isValidHostnameLabel(std::string_view{labelRange}))
                    return false;
            }

            return true;
        }

    }

    class Host {
    public:
        static std::expected<Host, std::error_code> create(std::string value) {
            if (value.empty())
                return std::unexpected(HostError::EmptyHost);

            boost::system::error_code ec;
            boost::asio::ip::make_address(value, ec);
            if (!ec)
                return Host{std::move(value)};

            if (detail::isValidHostname(value))
                return Host{std::move(value)};

            return std::unexpected(HostError::InvalidFormat);
        }

        const std::string& value() const {
            return _value;
        }

        bool operator==(const Host&) const = default;

    private:
        explicit Host(std::string value) : _value(std::move(value)) {
        }

    private:
        std::string _value;
    };

}

#endif //LIQUIDPETPROJECT_HOST_H

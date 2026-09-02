//
// Created by DED on 22.08.2026.
//

#ifndef LIQUIDPETPROJECT_NETWORKADDRESS_H
#define LIQUIDPETPROJECT_NETWORKADDRESS_H

#include <expected>
#include <string>
#include <system_error>
#include <boost/asio/ip/address.hpp>

namespace order_system::models {

    enum class NetworkAddressError : std::uint8_t {
        EmptyAddress = 1,
        InvalidFormat
    };

    class NetworkAddressErrorCategory : public std::error_category {
    public:
        const char* name() const noexcept override { return "networkAddress"; }

        std::string message(int ev) const override {
            switch (static_cast<NetworkAddressError>(ev)) {
                case NetworkAddressError::EmptyAddress:
                    return "empty address";
                case NetworkAddressError::InvalidFormat:
                    return "invalid format";
                default:
                    return "unknown networkAddress error";
            }
        }
    };

    inline const NetworkAddressErrorCategory& networkAddressErrorCategory() {
        static NetworkAddressErrorCategory instance;
        return instance;
    }

    inline std::error_code make_error_code(NetworkAddressError e) {
        return {static_cast<int>(e), networkAddressErrorCategory()};
    }

}

namespace std {
    template <>
    struct is_error_code_enum<order_system::models::NetworkAddressError> : true_type {
    };
}

namespace order_system::models {
    class NetworkAddress {
    public:
        static std::expected<NetworkAddress, std::error_code> create(std::string value) {
            if (value.empty())
                return std::unexpected(NetworkAddressError::EmptyAddress);

            boost::system::error_code ec;
            boost::asio::ip::make_address(value, ec);
            if (ec)
                return std::unexpected(NetworkAddressError::InvalidFormat);

            return NetworkAddress{std::move(value)};
        }

        const std::string& value() const {
            return _value;
        }

        bool operator==(const NetworkAddress&) const = default;

    private:
        explicit NetworkAddress(std::string value) : _value(std::move(value)) {
        }

    private:

        std::string _value;
    };
}

#endif //LIQUIDPETPROJECT_NETWORKADDRESS_H

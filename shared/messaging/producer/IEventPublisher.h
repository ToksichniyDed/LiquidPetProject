//
// Created by DED on 06.09.2026.
//

#ifndef LIQUIDPETPROJECT_IEVENTPUBLISHER_H
#define LIQUIDPETPROJECT_IEVENTPUBLISHER_H

#include <string>
#include <expected>
#include <utility>

#include <PublishRequest.h>

namespace shared::messaging {
    enum class EventPublisherError : std::uint8_t {
        ConnectionFailure = 1,
        Timeout,
        SerializationFailure,
        BrokerRejected
    };

    class EventPublisherErrorCategory : public std::error_category {
    public:
        const char* name() const noexcept override { return "event_publisher"; }

        std::string message(int ev) const override {
            switch (static_cast<EventPublisherError>(ev)) {
                case EventPublisherError::ConnectionFailure:
                    return "connection failure";
                case EventPublisherError::Timeout:
                    return "timeout";
                case EventPublisherError::SerializationFailure:
                    return "serialization failure";
                case EventPublisherError::BrokerRejected:
                    return "broker rejected message";
                default:
                    return "unknown event publisher error";
            }
        }
    };

    inline const EventPublisherErrorCategory& eventPublisherErrorCategory() {
        static EventPublisherErrorCategory instance;
        return instance;
    }

    inline std::error_code make_error_code(EventPublisherError e) {
        return std::error_code(std::to_underlying(e), eventPublisherErrorCategory());
    }
}

namespace std {
    template <>
    struct is_error_code_enum<shared::messaging::EventPublisherError> : true_type {
    };
}

namespace shared::messaging {
    class IEventPublisher {
    public:
        virtual ~IEventPublisher() = default;

        [[nodiscard]] virtual std::future<std::expected<void, std::error_code>> publish(
            const PublishRequest& request) = 0;
    };

}


#endif //LIQUIDPETPROJECT_IEVENTPUBLISHER_H

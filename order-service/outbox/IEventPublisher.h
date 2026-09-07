//
// Created by DED on 06.09.2026.
//

#ifndef LIQUIDPETPROJECT_IEVENTPUBLISHER_H
#define LIQUIDPETPROJECT_IEVENTPUBLISHER_H

#include <string>
#include <expected>

namespace order_service::messaging {
    enum class EventPublisherError {
        ConnectionFailure = 1,
        Timeout,
        SerializationFailure,
        BrokerRejected
    };

    class EventPublisherErrorCategory : public std::error_category {
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
        return std::error_code(static_cast<int>(e), eventPublisherErrorCategory());
    }
}

namespace std {
    template <>
    struct is_error_code_enum<order_service::messaging::EventPublisherError> : true_type {
    };
}

namespace order_service::messaging {
    class IEventPublisher {
    public:
        virtual ~IEventPublisher() = default;

        [[nodiscard]] virtual std::future<std::expected<void, std::error_code>> publish(
            const std::string& topic,
            const std::string& key,
            const std::string& payload) = 0;
    };

}


#endif //LIQUIDPETPROJECT_IEVENTPUBLISHER_H

//
// Created by DED on 08.09.2026.
//

#ifndef LIQUIDPETPROJECT_IEVENTCONSUMER_H
#define LIQUIDPETPROJECT_IEVENTCONSUMER_H

#include <expected>
#include <functional>
#include <string>
#include <utility>
#include <cstdint>

#include "IEventHandler.h"

namespace shared::messaging {

enum class EventConsumerError : std::uint8_t {
    ConnectionFailure = 1,
    Timeout,
    BrokerRejected
};

class EventConsumerErrorCategory : public std::error_category {
   public:
    const char* name() const noexcept override { return "event_consumer"; }

    std::string message(int ev) const override {
        switch (static_cast<EventConsumerError>(ev)) {
            case EventConsumerError::ConnectionFailure:
                return "connection failure";
            case EventConsumerError::Timeout:
                return "timeout";
            case EventConsumerError::BrokerRejected:
                return "broker rejected message";
            default:
                return "unknown event consumer error";
        }
    }
};

inline const EventConsumerErrorCategory& eventConsumerErrorCategory() {
    static EventConsumerErrorCategory instance;
    return instance;
}

inline std::error_code make_error_code(EventConsumerError e) {
    return std::error_code(std::to_underlying(e), eventConsumerErrorCategory());
}

}  // namespace shared::messaging

namespace std {
template <>
struct is_error_code_enum<shared::messaging::EventConsumerError> : true_type {};
}  // namespace std

namespace shared::messaging {

class IEventConsumer {
   public:
    virtual ~IEventConsumer() = default;

    // Запускает consumer loop в фоне (собственный поток), вызывает handler
    // на каждое полученное сообщение. Неблокирующий вызов.
    [[nodiscard]] virtual std::expected<void, std::error_code> start(IEventHandler& handler) = 0;

    // Останавливает consumer loop, дожидается завершения текущей итерации.
    virtual void stop() = 0;
};

}  // namespace shared::messaging

#endif  // LIQUIDPETPROJECT_IEVENTCONSUMER_H

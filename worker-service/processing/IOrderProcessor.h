//
// Created by DED on 10.09.2026.
//

#ifndef LIQUIDPETPROJECT_IORDERPROCESSOR_H
#define LIQUIDPETPROJECT_IORDERPROCESSOR_H

#include "OrderCreatedEvent.h"

namespace worker_service::processing {

enum class ProcessingError : std::uint8_t { ReservationFailed = 1 };

class ProcessingErrorCategory : public std::error_category {
public:
    const char* name() const noexcept override { return "order_processing"; }

    std::string message(int ev) const override {
        switch (static_cast<ProcessingError>(ev)) {
            case ProcessingError::ReservationFailed:
                return "reservation failed";
            default:
                return "unknown processing error";
        }
    }
};

inline const ProcessingErrorCategory& processingErrorCategory() {
    static ProcessingErrorCategory instance;
    return instance;
}

inline std::error_code make_error_code(ProcessingError e) {
    return std::error_code(static_cast<int>(e), processingErrorCategory());
}

}  // namespace worker_service::processing

namespace std {
template <>
struct is_error_code_enum<worker_service::processing::ProcessingError> : true_type {};
}

namespace worker_service::processing {

class IOrderProcessor {
public:
    virtual ~IOrderProcessor() = default;

    [[nodiscard]] virtual std::expected<void, std::error_code> process(const events::OrderCreatedEvent& event) = 0;
};

}

#endif //LIQUIDPETPROJECT_IORDERPROCESSOR_H

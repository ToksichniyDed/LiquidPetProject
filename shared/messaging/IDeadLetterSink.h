//
// Created by DED on 01.10.2026.
//

#ifndef LIQUIDPETPROJECT_IDEADLETTERSINK_H
#define LIQUIDPETPROJECT_IDEADLETTERSINK_H

#include <cstdint>
#include <expected>
#include <string>
#include <system_error>

#include "PartitionRegistry.h"
#include <messaging/MessageMetadata.h>

namespace shared::messaging {

struct DeadLetterRecord {
    PartitionKey source;  // откуда пришла запись
    std::int64_t offset;
    int attempts;  // сколько раз пробовали обработать
    std::string payload;
    MessageMetadata metadata;
};

class IDeadLetterSink {
public:
    virtual ~IDeadLetterSink() = default;

    // Успех = запись сохранена
    [[nodiscard]] virtual std::expected<void, std::error_code> send(const DeadLetterRecord& record) = 0;
};

}  // namespace shared::messaging

#endif  // LIQUIDPETPROJECT_IDEADLETTERSINK_H

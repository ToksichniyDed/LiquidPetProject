//
// Created by DED on 12.09.2026.
//

#ifndef LIQUIDPETPROJECT_MESSAGEMETADATA_H
#define LIQUIDPETPROJECT_MESSAGEMETADATA_H

#include <string>

#include <OrderIds.h>

namespace shared::messaging {

struct MessageMetadata
{
    models::OutboxEventId eventId;
    std::string eventType;

    bool operator==(const MessageMetadata&) const = default;
};

}

#endif  // LIQUIDPETPROJECT_MESSAGEMETADATA_H

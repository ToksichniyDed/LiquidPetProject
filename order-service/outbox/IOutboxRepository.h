//
// Created by DED on 06.09.2026.
//

#ifndef LIQUIDPETPROJECT_IOUTBOXREPOSITORY_H
#define LIQUIDPETPROJECT_IOUTBOXREPOSITORY_H

#include <string>
#include <vector>

namespace order_service::outbox {

    struct OutboxEntry {
        std::string id;
        std::string aggregateId;
        std::string eventType;
        std::string payload;
    };

    class IOutboxRepository {
    public:
        virtual ~IOutboxRepository() = default;

        virtual std::vector<OutboxEntry> fetchUnpublished(int limit) = 0;
        virtual void markAsPublished(const std::string& entryId) = 0;
    };
}

#endif //LIQUIDPETPROJECT_IOUTBOXREPOSITORY_H

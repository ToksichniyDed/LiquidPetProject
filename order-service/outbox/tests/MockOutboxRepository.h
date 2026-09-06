//
// Created by DED on 06.09.2026.
//

#ifndef LIQUIDPETPROJECT_MOCKOUTBOXREPOSITOTY_H
#define LIQUIDPETPROJECT_MOCKOUTBOXREPOSITOTY_H

#include <gmock/gmock.h>

#include "IOutboxRepository.h"

namespace order_service::outbox {

    class MockOutboxRepository : public IOutboxRepository {
    public:
        MOCK_METHOD((std::expected<std::vector<OutboxEntry>, std::error_code>), fetchUnpublished,
                    (int limit), (override));
        MOCK_METHOD((std::expected<void, std::error_code>), markAsPublished,
                    (const std::string& entryId), (override));
    };

}

#endif //LIQUIDPETPROJECT_MOCKOUTBOXREPOSITOTY_H

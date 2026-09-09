//
// Created by DED on 06.09.2026.
//

#ifndef LIQUIDPETPROJECT_OUTBOXREPOSITORYQUERIES_H
#define LIQUIDPETPROJECT_OUTBOXREPOSITORYQUERIES_H

namespace repository::queries {

    inline constexpr auto SELECT_UNPUBLISHED_OUTBOX = "select_unpublished_outbox";
    inline constexpr auto MARK_OUTBOX_PUBLISHED = "mark_outbox_published";

    inline constexpr auto SELECT_UNPUBLISHED_OUTBOX_SQL =
            "SELECT id, aggregate_id, event_type, payload "
            "FROM outbox "
            "WHERE published = FALSE "
            "ORDER BY created_at "
            "LIMIT $1 "
            "FOR UPDATE SKIP LOCKED";

    inline constexpr auto MARK_OUTBOX_PUBLISHED_SQL =
            "UPDATE outbox "
            "SET published = TRUE, published_at = now() "
            "WHERE id = $1";

}

#endif //LIQUIDPETPROJECT_OUTBOXREPOSITORYQUERIES_H

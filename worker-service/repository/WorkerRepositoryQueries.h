//
// Created by DED on 10.09.2026.
//

#ifndef LIQUIDPETPROJECT_WORKERREPOSITORYQUERIES_H
#define LIQUIDPETPROJECT_WORKERREPOSITORYQUERIES_H

namespace worker_service::repository::queries {

inline constexpr auto INSERT_PROCESSED_EVENT = "insert_processed_event";
inline constexpr auto INSERT_OUTBOX_EVENT = "insert_outbox_event";

inline constexpr auto INSERT_PROCESSED_EVENT_SQL =
    "INSERT INTO processed_order_events (event_id) "
    "VALUES ($1) "
    "ON CONFLICT DO NOTHING";
inline constexpr auto INSERT_OUTBOX_EVENT_SQL =
    "INSERT INTO outbox (aggregate_id, event_type, payload) "
    "VALUES ($1, $2, $3::jsonb)";

}  // namespace worker_service::repository::queries

#endif  // LIQUIDPETPROJECT_WORKERREPOSITORYQUERIES_H

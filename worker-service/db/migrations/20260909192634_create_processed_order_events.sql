-- migrate:up
CREATE TABLE processed_order_events (
                                        event_id BIGINT PRIMARY KEY,
                                        processed_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

-- migrate:down
DROP TABLE processed_order_events;

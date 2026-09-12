-- migrate:up
DROP TABLE IF EXISTS processed_order_events;

CREATE TABLE processed_order_events (
                                        event_id UUID PRIMARY KEY,
                                        processed_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

-- migrate:down
DROP TABLE processed_order_events;

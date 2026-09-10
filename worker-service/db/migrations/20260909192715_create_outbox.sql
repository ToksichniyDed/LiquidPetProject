-- migrate:up
CREATE TABLE outbox (
                        id BIGSERIAL PRIMARY KEY,
                        event_type TEXT NOT NULL,
                        payload JSONB NOT NULL,
                        published BOOLEAN NOT NULL DEFAULT FALSE,
                        created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX idx_outbox_unpublished ON outbox (published) WHERE published = FALSE;

-- migrate:down
DROP TABLE outbox;

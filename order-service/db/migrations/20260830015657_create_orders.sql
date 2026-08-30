-- migrate:up
CREATE TABLE orders
(
    order_id   UUID PRIMARY KEY     DEFAULT gen_random_uuid(),
    user_id    UUID        NOT NULL,
    status     TEXT        NOT NULL DEFAULT 'Created'
        CHECK (status IN ('Created', 'Reserved', 'Shipped', 'Cancelled')),
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX idx_orders_user_id ON orders (user_id);

CREATE
OR REPLACE FUNCTION set_updated_at()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at
= now();
RETURN NEW;
END;
$$
LANGUAGE plpgsql;

CREATE TRIGGER orders_set_updated_at
    BEFORE UPDATE
    ON orders
    FOR EACH ROW
    EXECUTE FUNCTION set_updated_at();

-- migrate:down
DROP TRIGGER IF EXISTS orders_set_updated_at ON orders;
DROP FUNCTION IF EXISTS set_updated_at();
DROP TABLE orders;

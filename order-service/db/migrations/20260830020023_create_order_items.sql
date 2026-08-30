-- migrate:up
CREATE TABLE order_items
(
    id                          BIGSERIAL PRIMARY KEY,
    order_id                    UUID     NOT NULL REFERENCES orders (order_id) ON DELETE CASCADE,
    product_id                  UUID     NOT NULL,
    quantity                    INTEGER  NOT NULL CHECK (quantity > 0),
    price_minor_units           BIGINT   NOT NULL CHECK (price_minor_units >= 0),
    price_currency_code         TEXT     NOT NULL,
    price_currency_minor_digits SMALLINT NOT NULL
);

CREATE INDEX idx_order_items_order_id ON order_items (order_id);

-- migrate:down
DROP TABLE order_items;

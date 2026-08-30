//
// Created by DED on 30.08.2026.
//

#ifndef LIQUIDPETPROJECT_ORDERREPOSITORYQUERIES_H
#define LIQUIDPETPROJECT_ORDERREPOSITORYQUERIES_H

namespace order_system::repository::queries {

    inline constexpr auto INSERT_ORDER = "insert_order";
    inline constexpr auto INSERT_ORDER_ITEM = "insert_order_item";
    inline constexpr auto SELECT_ORDER = "select_order";
    inline constexpr auto SELECT_ORDER_ITEMS = "select_order_items";

    inline constexpr auto INSERT_ORDER_SQL =
            "INSERT INTO orders (user_id, status) "
            "VALUES ($1, $2) "
            "RETURNING order_id";

    inline constexpr auto INSERT_ORDER_ITEM_SQL =
            "INSERT INTO order_items "
            "(order_id, product_id, quantity, price_minor_units, price_currency_code, price_currency_minor_digits) "
            "VALUES ($1, $2, $3, $4, $5, $6)";

    inline constexpr auto SELECT_ORDER_SQL =
            "SELECT order_id, user_id, status "
            "FROM orders "
            "WHERE order_id = $1";

    inline constexpr auto SELECT_ORDER_ITEMS_SQL =
            "SELECT product_id, quantity, price_minor_units, price_currency_code, price_currency_minor_digits "
            "FROM order_items "
            "WHERE order_id = $1";

}

#endif //LIQUIDPETPROJECT_ORDERREPOSITORYQUERIES_H

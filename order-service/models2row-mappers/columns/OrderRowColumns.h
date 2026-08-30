//
// Created by DED on 30.08.2026.
//

#ifndef LIQUIDPETPROJECT_ORDERROWCOLUMNS_H
#define LIQUIDPETPROJECT_ORDERROWCOLUMNS_H

namespace order_system::repository::columns {

    // orders
    inline constexpr auto ORDER_ID = "order_id";
    inline constexpr auto ORDER_USER_ID = "user_id";
    inline constexpr auto ORDER_STATUS = "status";

    // order_items
    inline constexpr auto ORDER_ITEM_PRODUCT_ID = "product_id";
    inline constexpr auto ORDER_ITEM_QUANTITY = "quantity";
    inline constexpr auto ORDER_ITEM_PRICE_MINOR_UNITS = "price_minor_units";
    inline constexpr auto ORDER_ITEM_PRICE_CURRENCY_CODE = "price_currency_code";
    inline constexpr auto ORDER_ITEM_PRICE_CURRENCY_MINOR_DIGITS = "price_currency_minor_digits";

}

#endif //LIQUIDPETPROJECT_ORDERROWCOLUMNS_H

//
// Created by DED on 29.08.2026.
//

#ifndef LIQUIDPETPROJECT_ORDERJSONKEYS_H
#define LIQUIDPETPROJECT_ORDERJSONKEYS_H

namespace order_system::models2json_mapper::keys {

    // Order
    inline constexpr auto ORDER_ID = "orderId";
    inline constexpr auto ORDER_USER_ID = "userId";
    inline constexpr auto ORDER_ITEMS = "items";
    inline constexpr auto ORDER_STATUS = "status";

    // OrderItem
    inline constexpr auto ORDER_ITEM_PRODUCT_ID = "productId";
    inline constexpr auto ORDER_ITEM_QUANTITY = "quantity";
    inline constexpr auto ORDER_ITEM_PRICE_AT_ORDER_TIME = "priceAtOrderTime";

}
#endif //LIQUIDPETPROJECT_ORDERJSONKEYS_H

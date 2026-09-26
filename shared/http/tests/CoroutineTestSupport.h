//
// Created by DED on 26.09.2026.
//

#ifndef LIQUIDPETPROJECT_COROUTINETESTSUPPORT_H
#define LIQUIDPETPROJECT_COROUTINETESTSUPPORT_H

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>

namespace shared::http::tests {

// Синхронно "прогоняет" awaitable-корутину до конца и возвращает результат.
// Используется только в тестах: продакшен-код никогда не должен блокирующе
// ждать корутину - это свело бы на нет весь смысл асинхронности.
template <typename Awaitable>
auto runSync(boost::asio::io_context& ioContext, Awaitable&& awaitable) {
    auto future = boost::asio::co_spawn(
        ioContext, std::forward<Awaitable>(awaitable), boost::asio::use_future);

    ioContext.run();    // крутим до тех пор, пока корутина не завершится
    ioContext.restart(); // io_context после run() до конца "истощён" - готовим к переиспользованию

    return future.get(); // если корутина бросила исключение - оно вылетит здесь
}

}

#endif //LIQUIDPETPROJECT_COROUTINETESTSUPPORT_H

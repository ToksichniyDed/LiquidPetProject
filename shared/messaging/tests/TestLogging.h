//
// Created by DED on 02.10.2026.
//

#ifndef LIQUIDPETPROJECT_TESTLOGGING_H
#define LIQUIDPETPROJECT_TESTLOGGING_H

#include <logging/Logger.h>

namespace shared::messaging::tests {

// inline-переменная с динамической инициализацией: выполняется один раз до main, сколько бы TU ни включали заголовок
inline const bool LOGGING_INITIALIZED = [] {
    shared::logger::init(true, false, spdlog::level::warn, {}, 1024, 0);
    return true;
}();

}  // namespace shared::messaging::tests

#endif  // LIQUIDPETPROJECT_TESTLOGGING_H

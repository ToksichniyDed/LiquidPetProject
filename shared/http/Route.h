//
// Created by DED on 29.08.2026.
//

#ifndef LIQUIDPETPROJECT_ROUTE_H
#define LIQUIDPETPROJECT_ROUTE_H

#include <string>
#include <memory>

#include "IRequestHandler.h"

namespace shared::http::handlers {
    struct Route {
        Method method;
        std::string pathPrefix;
        std::shared_ptr<IRequestHandler> handler;
    };
}


#endif //LIQUIDPETPROJECT_ROUTE_H

//
// Created by DED on 29.08.2026.
//

#ifndef LIQUIDPETPROJECT_ROUTE_H
#define LIQUIDPETPROJECT_ROUTE_H

#include <string>
#include <memory>

#include "IRequestHandler.h"

namespace order_service::handlers {
    struct Route {
        Http::Method method;
        std::string pathPrefix;
        std::shared_ptr<IRequestHandler> handler;
    };
}


#endif //LIQUIDPETPROJECT_ROUTE_H

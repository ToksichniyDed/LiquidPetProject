//
// Created by DED on 27.08.2026.
//

#ifndef LIQUIDPETPROJECT_IREQUESTHANDLER_H
#define LIQUIDPETPROJECT_IREQUESTHANDLER_H

#include "http/HttpMessage.h"

namespace order_service::handlers {
   class IRequestHandler {
public:
       virtual ~IRequestHandler() = default;
       virtual Http::Response handle(const Http::Request& request) = 0;
};
}

#endif //LIQUIDPETPROJECT_IREQUESTHANDLER_H

//
// Created by DED on 27.08.2026.
//

#ifndef LIQUIDPETPROJECT_IREQUESTHANDLER_H
#define LIQUIDPETPROJECT_IREQUESTHANDLER_H

#include <models/HttpMessage.h>

namespace shared::http {
    class IRequestHandler {
public:
       virtual ~IRequestHandler() = default;
       virtual models::Response handle(const models::Request& request) = 0;
};
}

#endif //LIQUIDPETPROJECT_IREQUESTHANDLER_H

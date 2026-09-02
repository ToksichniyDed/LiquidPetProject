//
// Created by DED on 27.08.2026.
//

#ifndef LIQUIDPETPROJECT_HTTPMESSAGE_H
#define LIQUIDPETPROJECT_HTTPMESSAGE_H

#include <string>

namespace Http {
    enum class Method : std::uint8_t {
        Get,
        Post,
        Unknown
    };

    enum class Status : std::uint16_t {
        Ok = 200,
        Created = 201,
        BadRequest = 400,
        NotFound = 404,
        MethodNotAllowed = 405,
        IAmATeaPot = 418,
        InternalServerError = 500
    };

    struct Request {
        Method method;
        std::string path;
        std::string body;
    };

    struct Response {
        Status status;
        std::string body;
    };
}


#endif //LIQUIDPETPROJECT_HTTPMESSAGE_H

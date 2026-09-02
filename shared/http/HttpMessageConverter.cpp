//
// Created by DED on 27.08.2026.
//

#include "HttpMessageConverter.h"

#include <utility>

namespace order_service::handlers {
    Http::Request HttpMessageConverter::toHttpRequest(const http::request<http::string_body>& request) {
        const auto method = toHttpMethod(request.method());

        return {.method=method, .path=std::string(request.target()),.body=request.body()};
    }

    http::response<http::string_body> HttpMessageConverter::toBeastResponse(const Http::Response& response) {
        http::response<http::string_body> beastResponse;

        beastResponse.result(static_cast<http::status>(std::to_underlying(response.status)));
        beastResponse.body() = response.body;
        beastResponse.prepare_payload();

        return beastResponse;
    }

    Http::Method HttpMessageConverter::toHttpMethod(const http::verb verb) {
        switch (verb) {
            using enum Http::Method;
            case http::verb::get:
                return Get;
            case http::verb::post:
                return Post;
            default:
                return Unknown;
        }
    }
}

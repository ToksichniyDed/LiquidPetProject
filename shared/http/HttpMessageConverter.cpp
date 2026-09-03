//
// Created by DED on 27.08.2026.
//

#include "HttpMessageConverter.h"

#include <utility>

namespace shared::http {
    Request HttpMessageConverter::toHttpRequest(
        const boost::beast::http::request<boost::beast::http::string_body>& request) {
        const auto method = toHttpMethod(request.method());

        return {.method=method, .path=std::string(request.target()),.body=request.body()};
    }

    boost::beast::http::response<boost::beast::http::string_body> HttpMessageConverter::toBeastResponse(
        const Response& response) {
        boost::beast::http::response<boost::beast::http::string_body> beastResponse;

        beastResponse.result(static_cast<boost::beast::http::status>(std::to_underlying(response.status)));
        beastResponse.body() = response.body;
        beastResponse.prepare_payload();

        return beastResponse;
    }

    boost::beast::http::request<boost::beast::http::string_body> HttpMessageConverter::toBeastRequest(
        const Request& request) {
        boost::beast::http::request<boost::beast::http::string_body> beastRequest;

        beastRequest.method(toVerb(request.method));
        beastRequest.target(request.path);
        beastRequest.body() = request.body;
        beastRequest.set(boost::beast::http::field::content_type, "application/json");
        beastRequest.prepare_payload();

        return beastRequest;
    }

    Response HttpMessageConverter::toHttpResponse(const boost::beast::http::response<boost::beast::http::string_body>& response) {

        return {
            .status = static_cast<Status>(response.result_int()),
            .body = response.body()
        };
    }

    boost::beast::http::verb HttpMessageConverter::toVerb(Method method) {
        switch (method) {
                using enum Method;
            case Get:
                return boost::beast::http::verb::get;
            case Post:
                return boost::beast::http::verb::post;
            default:
                return boost::beast::http::verb::unknown;
        }
    }

    Method HttpMessageConverter::toHttpMethod(const boost::beast::http::verb verb) {
        switch (verb) {
                using enum Method;
            case boost::beast::http::verb::get:
                return Get;
            case boost::beast::http::verb::post:
                return Post;
            default:
                return Unknown;
        }
    }
}

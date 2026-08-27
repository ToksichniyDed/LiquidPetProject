//
// Created by DED on 27.08.2026.
//

#ifndef LIQUIDPETPROJECT_HTTPMESSAGECONVERTER_H
#define LIQUIDPETPROJECT_HTTPMESSAGECONVERTER_H

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>

#include <http/HttpMessage.h>

namespace order_service::handlers {
    namespace beast = boost::beast;
    namespace http = beast::http;

    class HttpMessageConverter {
    public:
        static Http::Request toHttpRequest(const http::request<http::string_body>& request);
        static http::response<http::string_body> toBeastResponse(const Http::Response& response);
        static Http::Method toHttpMethod(http::verb verb);
    };
}



#endif //LIQUIDPETPROJECT_HTTPMESSAGECONVERTER_H

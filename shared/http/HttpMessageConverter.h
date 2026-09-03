//
// Created by DED on 27.08.2026.
//

#ifndef LIQUIDPETPROJECT_HTTPMESSAGECONVERTER_H
#define LIQUIDPETPROJECT_HTTPMESSAGECONVERTER_H

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>

#include <http/HttpMessage.h>

namespace shared::http {
    class HttpMessageConverter {
    public:
        static Request toHttpRequest(const boost::beast::http::request<boost::beast::http::string_body>& request);
        static boost::beast::http::request<boost::beast::http::string_body> toBeastRequest(const Request& request);

        static boost::beast::http::response<boost::beast::http::string_body> toBeastResponse(const Response& response);
        static Response toHttpResponse(const boost::beast::http::response<boost::beast::http::string_body>& response);

        static Method toHttpMethod(boost::beast::http::verb verb);
        static boost::beast::http::verb toVerb(Method method);
    };
}



#endif //LIQUIDPETPROJECT_HTTPMESSAGECONVERTER_H

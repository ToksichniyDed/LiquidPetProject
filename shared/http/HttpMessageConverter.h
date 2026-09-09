//
// Created by DED on 27.08.2026.
//

#ifndef LIQUIDPETPROJECT_HTTPMESSAGECONVERTER_H
#define LIQUIDPETPROJECT_HTTPMESSAGECONVERTER_H

#include <models/HttpMessage.h>

#include <boost/beast/http.hpp>

namespace shared::http {
    class HttpMessageConverter {
    public:
        static models::Request toHttpRequest(const boost::beast::http::request<boost::beast::http::string_body>& request);
        static boost::beast::http::request<boost::beast::http::string_body> toBeastRequest(
            const models::Request& request);

        static boost::beast::http::response<boost::beast::http::string_body> toBeastResponse(
            const models::Response& response);
        static models::Response toHttpResponse(const boost::beast::http::response<boost::beast::http::string_body>& response);

        static models::Method toHttpMethod(boost::beast::http::verb verb);
        static boost::beast::http::verb toVerb(models::Method method);
    };
}



#endif //LIQUIDPETPROJECT_HTTPMESSAGECONVERTER_H

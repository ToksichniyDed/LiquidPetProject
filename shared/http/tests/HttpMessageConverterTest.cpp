//
// Created by DED on 03.09.2026.
//

#include <gtest/gtest.h>
#include <http/HttpMessageConverter.h>
#include <http/HttpMessage.h>

namespace shared::http::tests {

    namespace beast_http = boost::beast::http;

    TEST(HttpMessageConverterTest, ToHttpRequest_MapsGetMethod) {
        beast_http::request<beast_http::string_body> beastRequest{beast_http::verb::get, "/orders/123", 11};

        const auto request = HttpMessageConverter::toHttpRequest(beastRequest);

        EXPECT_EQ(request.method, Method::Get);
    }

    TEST(HttpMessageConverterTest, ToHttpRequest_MapsPostMethod) {
        beast_http::request<beast_http::string_body> beastRequest{beast_http::verb::post, "/orders", 11};

        const auto request = HttpMessageConverter::toHttpRequest(beastRequest);

        EXPECT_EQ(request.method, Method::Post);
    }

    TEST(HttpMessageConverterTest, ToHttpRequest_UnsupportedMethodMapsToUnknown) {
        beast_http::request<beast_http::string_body> beastRequest{beast_http::verb::delete_, "/orders/123", 11};

        const auto request = HttpMessageConverter::toHttpRequest(beastRequest);

        EXPECT_EQ(request.method, Method::Unknown);
    }

    TEST(HttpMessageConverterTest, ToHttpRequest_CopiesPathAndBody) {
        beast_http::request<beast_http::string_body> beastRequest{beast_http::verb::post, "/orders", 11};
        beastRequest.body() = R"({"userId":"abc"})";

        const auto request = HttpMessageConverter::toHttpRequest(beastRequest);

        EXPECT_EQ(request.path, "/orders");
        EXPECT_EQ(request.body, R"({"userId":"abc"})");
    }

    TEST(HttpMessageConverterTest, ToBeastResponse_MapsStatusCode) {
        const Response response{.status = Status::Created, .body = ""};

        const auto beastResponse = HttpMessageConverter::toBeastResponse(response);

        EXPECT_EQ(beastResponse.result_int(), 201);
    }

    TEST(HttpMessageConverterTest, ToBeastResponse_CopiesBody) {
        const Response response{.status = Status::Ok, .body = R"({"id":"xyz"})"};

        const auto beastResponse = HttpMessageConverter::toBeastResponse(response);

        EXPECT_EQ(beastResponse.body(), R"({"id":"xyz"})");
    }

    TEST(HttpMessageConverterTest, ToBeastResponse_SetsContentLength) {
        const Response response{.status = Status::Ok, .body = "hello"};

        const auto beastResponse = HttpMessageConverter::toBeastResponse(response);

        EXPECT_TRUE(beastResponse.has_content_length());
    }

    TEST(HttpMessageConverterTest, ToBeastRequest_MapsGetMethod) {
        const Request request{.method = Method::Get, .path = "/orders/123", .body = ""};

        const auto beastRequest = HttpMessageConverter::toBeastRequest(request);

        EXPECT_EQ(beastRequest.method(), beast_http::verb::get);
    }

    TEST(HttpMessageConverterTest, ToBeastRequest_MapsPostMethod) {
        const Request request{.method = Method::Post, .path = "/orders", .body = "{}"};

        const auto beastRequest = HttpMessageConverter::toBeastRequest(request);

        EXPECT_EQ(beastRequest.method(), beast_http::verb::post);
    }

    TEST(HttpMessageConverterTest, ToBeastRequest_UnknownMethodThrows) {
        const Request request{.method = Method::Unknown, .path = "/orders", .body = ""};

        EXPECT_THROW(HttpMessageConverter::toBeastRequest(request), std::invalid_argument);
    }

    TEST(HttpMessageConverterTest, ToBeastRequest_CopiesTargetAndBody) {
        const Request request{.method = Method::Post, .path = "/orders", .body = R"({"userId":"abc"})"};

        const auto beastRequest = HttpMessageConverter::toBeastRequest(request);

        EXPECT_EQ(beastRequest.target(), "/orders");
        EXPECT_EQ(beastRequest.body(), R"({"userId":"abc"})");
    }

    TEST(HttpMessageConverterTest, ToBeastRequest_SetsJsonContentType) {
        const Request request{.method = Method::Post, .path = "/orders", .body = "{}"};

        const auto beastRequest = HttpMessageConverter::toBeastRequest(request);

        const auto contentType = beastRequest[beast_http::field::content_type];
        EXPECT_EQ(contentType, "application/json");
    }

    TEST(HttpMessageConverterTest, ToHttpResponse_MapsKnownStatus) {
        beast_http::response<beast_http::string_body> beastResponse;
        beastResponse.result(beast_http::status::not_found);

        const auto response = HttpMessageConverter::toHttpResponse(beastResponse);

        EXPECT_EQ(response.status, Status::NotFound);
    }

    TEST(HttpMessageConverterTest, ToHttpResponse_CopiesBody) {
        beast_http::response<beast_http::string_body> beastResponse;
        beastResponse.result(beast_http::status::ok);
        beastResponse.body() = R"({"id":"xyz"})";

        const auto response = HttpMessageConverter::toHttpResponse(beastResponse);

        EXPECT_EQ(response.body, R"({"id":"xyz"})");
    }

    TEST(HttpMessageConverterTest, ToHttpResponse_StatusOutsideEnumIsCastAnyway) {
        beast_http::response<beast_http::string_body> beastResponse;
        beastResponse.result(beast_http::status::no_content);

        const auto response = HttpMessageConverter::toHttpResponse(beastResponse);

        EXPECT_EQ(static_cast<int>(response.status), 204);
    }

    TEST(HttpMessageConverterTest, RequestRoundTrip_PreservesMethodPathBody) {
        const Request original{.method = Method::Post, .path = "/orders", .body = R"({"a":1})"};

        const auto beastRequest = HttpMessageConverter::toBeastRequest(original);
        const auto roundTripped = HttpMessageConverter::toHttpRequest(beastRequest);

        EXPECT_EQ(roundTripped.method, original.method);
        EXPECT_EQ(roundTripped.path, original.path);
        EXPECT_EQ(roundTripped.body, original.body);
    }

    TEST(HttpMessageConverterTest, ResponseRoundTrip_PreservesStatusAndBody) {
        const Response original{.status = Status::Created, .body = R"({"id":"xyz"})"};

        const auto beastResponse = HttpMessageConverter::toBeastResponse(original);
        const auto roundTripped = HttpMessageConverter::toHttpResponse(beastResponse);

        EXPECT_EQ(roundTripped.status, original.status);
        EXPECT_EQ(roundTripped.body, original.body);
    }
}

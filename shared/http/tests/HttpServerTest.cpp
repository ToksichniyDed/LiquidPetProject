//
// Created by DED on 04.09.2026.
//

#include <gtest/gtest.h>
#include <thread>
#include <boost/beast.hpp>
#include <boost/asio.hpp>

#include <http/HttpServer.h>
#include <http/Route.h>
#include <http/NetworkConfiguration.h>
#include <logging/Logger.h>

namespace shared::http::tests {

    namespace beast = boost::beast;
    namespace beast_http = beast::http;
    using tcp = boost::asio::ip::tcp;

    class EchoHandler : public IRequestHandler {
    public:
        Response handle(const Request& request) override {
            return {.status = Status::Ok, .body = request.body};
        }
    };

    class HealthHandler : public IRequestHandler {
    public:
        Response handle(const Request&) override {
            return {.status = Status::Ok, .body = "ok"};
        }
    };

    beast_http::response<beast_http::string_body> sendRequest(
        const std::string& host, std::uint16_t port,
        beast_http::verb method, const std::string& target, const std::string& body = "") {

        boost::asio::io_context ioContext;
        tcp::resolver resolver(ioContext);
        tcp::socket socket(ioContext);

        const auto endpoints = resolver.resolve(host, std::to_string(port));
        boost::asio::connect(socket, endpoints);

        beast_http::request<beast_http::string_body> request{method, target, 11};
        request.set(beast_http::field::host, host);
        request.body() = body;
        request.prepare_payload();

        beast_http::write(socket, request);

        beast::flat_buffer buffer;
        beast_http::response<beast_http::string_body> response;
        beast_http::read(socket, buffer, response);

        boost::system::error_code ec;
        socket.shutdown(tcp::socket::shutdown_both, ec);

        return response;
    }

    class HttpServerTest : public ::testing::Test {
    protected:
        static void SetUpTestSuite() {
            Logger::init(true, false, spdlog::level::level_enum::debug, {}, 1024, 0);
        }

        void SetUp() override {
            auto address = models::NetworkAddress::create("127.0.0.1");
            ASSERT_TRUE(address.has_value());

            models::NetworkConfiguration config{.address = address.value(), .port = testPort};

            std::vector<handlers::Route> routes = {
                {shared::http::Method::Get, "/health", std::make_shared<HealthHandler>()},
                {shared::http::Method::Post, "/echo", std::make_shared<EchoHandler>()},
            };

            server = std::make_unique<HttpServer>(std::move(config), std::move(routes));
            serverThread = std::thread([this] { server->run(); });

            // дать серверу время забиндиться и войти в accept, костыль
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        void TearDown() override {
            server->stop();
            if (serverThread.joinable()) {
                serverThread.join();
            }
        }

        static constexpr std::uint16_t testPort = 18081;
        std::unique_ptr<HttpServer> server;
        std::thread serverThread;
    };

    TEST_F(HttpServerTest, RespondsToHealthCheck) {
        const auto response = sendRequest("127.0.0.1", testPort, beast_http::verb::get, "/health");

        EXPECT_EQ(response.result_int(), 200);
        EXPECT_EQ(response.body(), "ok");
    }

    TEST_F(HttpServerTest, EchoesPostBody) {
        const auto response = sendRequest("127.0.0.1", testPort, beast_http::verb::post, "/echo", "hello world");

        EXPECT_EQ(response.result_int(), 200);
        EXPECT_EQ(response.body(), "hello world");
    }

    TEST_F(HttpServerTest, ReturnsNotFoundForUnknownPath) {
        const auto response = sendRequest("127.0.0.1", testPort, beast_http::verb::get, "/unknown");

        EXPECT_EQ(response.result_int(), 404);
    }

    TEST_F(HttpServerTest, StopClosesAcceptorAndThreadJoinsCleanly) {
        server->stop();
        serverThread.join();
    }
}

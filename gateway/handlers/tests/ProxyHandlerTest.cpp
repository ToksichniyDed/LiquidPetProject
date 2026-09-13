//
// Created by DED on 13.09.2026.
//

#include <gtest/gtest.h>
#include <http/HttpServer.h>
#include <http/Route.h>
#include <logging/Logger.h>
#include <models/NetworkAddress.h>

#include <chrono>
#include <thread>

#include "../ProxyHandler.h"

using namespace gateway_service::handlers;
using namespace shared::models;
using namespace shared::http;

namespace {

// Upstream-заглушка: эхо-хендлер, который возвращает статус в зависимости
// от пути, чтобы отличать "дошло до апстрима" от "проксирование само упало".
class UpstreamEchoHandler : public IRequestHandler {
   public:
    Response handle(const Request& request) override {
        if (request.path == "/orders/fail") {
            return {.status = Status::InternalServerError, .body = "upstream failed"};
        }
        return {.status = Status::Ok, .body = "echo:" + request.body};
    }
};

NetworkConfiguration makeLocalConfig(std::uint16_t port) {
    return NetworkConfiguration{.address = NetworkAddress::create("127.0.0.1").value(), .port = port};
}

}  // namespace

class ProxyHandlerTest : public ::testing::Test {
   protected:
    static void SetUpTestSuite() { shared::logger::init(true, false, spdlog::level::level_enum::debug, {}, 1024, 0); }

    void startUpstreamOn(std::uint16_t port) {
        upstreamPort = port;

        std::vector<handlers::Route> routes = {
            {.method = Method::Get, .pathPrefix = "/orders", .handler = std::make_shared<UpstreamEchoHandler>()},
            {.method = Method::Post, .pathPrefix = "/orders", .handler = std::make_shared<UpstreamEchoHandler>()},
        };

        upstreamServer = std::make_unique<HttpServer>(makeLocalConfig(port), std::move(routes));
        upstreamThread = std::thread([this] { upstreamServer->run(); });

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    void TearDown() override {
        if (upstreamServer) {
            upstreamServer->stop();
            if (upstreamThread.joinable()) {
                upstreamThread.join();
            }
        }
    }

    std::uint16_t upstreamPort = 0;
    std::unique_ptr<HttpServer> upstreamServer;
    std::thread upstreamThread;
};

TEST_F(ProxyHandlerTest, UnknownMethodReturnsMethodNotAllowedWithoutContactingUpstream) {
    // Порт не поднят вообще - если бы прокси всё равно пытался достучаться,
    // тест бы упал по BadGateway, а не по MethodNotAllowed.
    std::unordered_map<std::string, NetworkConfiguration> services{{"/orders", makeLocalConfig(19999)}};
    ProxyHandler proxy(std::move(services));

    const Request request{.method = Method::Unknown, .path = "/orders", .body = ""};
    const auto response = proxy.handle(request);

    EXPECT_EQ(response.status, Status::MethodNotAllowed);
}

TEST_F(ProxyHandlerTest, UnmatchedPathReturnsNotFound) {
    std::unordered_map<std::string, NetworkConfiguration> services{{"/orders", makeLocalConfig(19999)}};
    ProxyHandler proxy(std::move(services));

    const Request request{.method = Method::Get, .path = "/unknown-service", .body = ""};
    const auto response = proxy.handle(request);

    EXPECT_EQ(response.status, Status::NotFound);
}

TEST_F(ProxyHandlerTest, UnreachableUpstreamReturnsBadGateway) {
    // Ничего не слушает на этом порту => connect должен упасть.
    std::unordered_map<std::string, NetworkConfiguration> services{{"/orders", makeLocalConfig(19998)}};
    ProxyHandler proxy(std::move(services));

    const Request request{.method = Method::Get, .path = "/orders/123", .body = ""};
    const auto response = proxy.handle(request);

    EXPECT_EQ(response.status, Status::BadGateway);
}

TEST_F(ProxyHandlerTest, ForwardsGetRequestAndReturnsUpstreamResponse) {
    startUpstreamOn(19801);

    std::unordered_map<std::string, NetworkConfiguration> services{{"/orders", makeLocalConfig(upstreamPort)}};
    ProxyHandler proxy(std::move(services));

    const Request request{.method = Method::Get, .path = "/orders/123", .body = ""};
    const auto response = proxy.handle(request);

    EXPECT_EQ(response.status, Status::Ok);
    EXPECT_EQ(response.body, "echo:");
}

TEST_F(ProxyHandlerTest, ForwardsPostBodyToUpstream) {
    startUpstreamOn(19802);

    std::unordered_map<std::string, NetworkConfiguration> services{{"/orders", makeLocalConfig(upstreamPort)}};
    ProxyHandler proxy(std::move(services));

    const Request request{.method = Method::Post, .path = "/orders", .body = R"({"userId":"abc"})"};
    const auto response = proxy.handle(request);

    EXPECT_EQ(response.status, Status::Ok);
    EXPECT_EQ(response.body, R"(echo:{"userId":"abc"})");
}

TEST_F(ProxyHandlerTest, PropagatesUpstreamErrorStatus) {
    startUpstreamOn(19803);

    std::unordered_map<std::string, NetworkConfiguration> services{{"/orders", makeLocalConfig(upstreamPort)}};
    ProxyHandler proxy(std::move(services));

    const Request request{.method = Method::Get, .path = "/orders/fail", .body = ""};
    const auto response = proxy.handle(request);

    EXPECT_EQ(response.status, Status::InternalServerError);
}

TEST_F(ProxyHandlerTest, RoutesToCorrectServiceAmongMultiple) {
    startUpstreamOn(19804);

    std::unordered_map<std::string, NetworkConfiguration> services{
        {"/orders", makeLocalConfig(upstreamPort)},
        {"/search", makeLocalConfig(19997)}  // несуществующий, не должен быть задет
    };
    ProxyHandler proxy(std::move(services));

    const Request request{.method = Method::Get, .path = "/orders/456", .body = ""};
    const auto response = proxy.handle(request);

    EXPECT_EQ(response.status, Status::Ok);
}

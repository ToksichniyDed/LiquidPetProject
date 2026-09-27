//
// Created by DED on 03.09.2026.
//

#ifndef LIQUIDPETPROJECT_PROXYHANDLER_H
#define LIQUIDPETPROJECT_PROXYHANDLER_H

#include <models/UpstreamConfiguration.h>
#include <http/IRequestHandler.h>

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/beast/http.hpp>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace gateway_service::handlers {
class ProxyHandler : public shared::http::IRequestHandler {
public:
    explicit ProxyHandler(std::unordered_map<std::string, shared::models::UpstreamConfiguration> services);
    ~ProxyHandler() override = default;
    boost::asio::awaitable<shared::models::Response> handle(const shared::models::Request& request) override;

private:
    // Простаивающие TCP-соединения к конкретному апстриму (ключ - тот же
    // pathPrefix, что и в _servicesUrl), доступные для переиспользования
    // как HTTP keep-alive. Без верхней границы: если нагрузка спадёт,
    // лишние простаивающие сокеты просто останутся в памяти до следующего
    // всплеска
    struct IdlePool {
        std::mutex mutex;
        std::vector<boost::asio::ip::tcp::socket> sockets;
    };

    // Пытается вытащить готовый сокет из пула для данного апстрима.
    // Возвращает nullopt, если пул пуст - тогда вызывающий сам открывает
    // новое соединение. Быстрая операция под мьютексом, без co_await внутри.
    // pathPrefix обязан существовать в _pools (заполняется в конструкторе
    // по ключам _servicesUrl) - только читаем
    std::optional<boost::asio::ip::tcp::socket> acquirePooledSocket(const std::string& pathPrefix);

    // Открывает новое TCP-соединение к апстриму (resolve + connect).
    boost::asio::awaitable<boost::asio::ip::tcp::socket> openSocket(
        const shared::models::UpstreamConfiguration& config, boost::asio::any_io_executor executor);

    // Отправляет запрос и читает ответ на уже установленном соединении.
    // Бросает boost::system::system_error при любой сетевой ошибке.
    boost::asio::awaitable<boost::beast::http::response<boost::beast::http::string_body>> exchange(
        boost::asio::ip::tcp::socket& socket, const shared::models::Request& request, const std::string& host);

    // Возвращает сокет в пул для переиспользования, если апстрим не
    // попросил закрыть соединение
    void releaseSocketIfReusable(
        const std::string& pathPrefix, boost::asio::ip::tcp::socket socket,
        const boost::beast::http::response<boost::beast::http::string_body>& response);

private:
    std::unordered_map<std::string, shared::models::UpstreamConfiguration> _servicesUrl;
    std::unordered_map<std::string, IdlePool> _pools;
};
}

#endif //LIQUIDPETPROJECT_PROXYHANDLER_H

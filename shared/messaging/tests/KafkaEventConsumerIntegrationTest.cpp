//
// Created by DED on 19.09.2026.
//

// Интеграционный тест: требует поднятого Kafka (docker compose up -d --wait kafka).

#include <gtest/gtest.h>
#include <logging/Logger.h>
#include <messaging/consumer/KafkaEventConsumer.h>
#include <messaging/producer/KafkaEventPublisher.h>

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <map>
#include <mutex>

using namespace shared::messaging;
using namespace std::chrono_literals;

namespace {

// Обработчик с "сценарием": для payload можно задать, сколько первых попыток провалить
// (-1 = проваливать всегда)
class ScriptedHandler : public IEventHandler {
   public:
    explicit ScriptedHandler(std::map<std::string, int> failures = {}) : _failures(std::move(failures)) {}

    bool handle(const std::string& payload, const MessageMetadata&) override {
        std::lock_guard lock(_mutex);
        ++_attempts[payload];

        if (auto it = _failures.find(payload); it != _failures.end() && it->second != 0) {
            if (it->second > 0) --it->second;
            _condition.notify_all();
            return false;
        }

        _succeeded.push_back(payload);
        _condition.notify_all();
        return true;
    }

    template <typename Predicate>
    bool waitFor(Predicate predicate, std::chrono::seconds timeout) {
        std::unique_lock lock(_mutex);
        return _condition.wait_for(lock, timeout, [&] { return predicate(_succeeded, _attempts); });
    }

    std::vector<std::string> succeeded() const {
        std::lock_guard lock(_mutex);
        return _succeeded;
    }

    int attempts(const std::string& payload) const {
        std::lock_guard lock(_mutex);
        const auto it = _attempts.find(payload);
        return it == _attempts.end() ? 0 : it->second;
    }

   private:
    mutable std::mutex _mutex;
    std::condition_variable _condition;
    std::map<std::string, int> _failures;
    std::map<std::string, int> _attempts;
    std::vector<std::string> _succeeded;
};

}  // namespace

class KafkaEventConsumerIntegrationTest : public ::testing::Test {
   protected:
    static void SetUpTestSuite() { shared::logger::init(true, false, spdlog::level::debug, {}, 1024, 0); }

    void SetUp() override {
        const char* brokers = std::getenv("KAFKA_BROKERS");
        if (!brokers) {
            GTEST_SKIP() << "KAFKA_BROKERS не задан — пропускаем интеграционный тест. Пример: localhost:9092";
        }
        _brokers = brokers;

        // Уникальные топик и группа на каждый тест: прогоны не влияют друг на друга
        const auto suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        _topic = "test.consumer." + suffix;
        _groupId = "test-group-" + suffix;

        _publisher = std::make_unique<KafkaEventPublisher>(_brokers);
    }

    // Один и тот же ключ => одна партиция => порядок сообщений гарантирован
    void publish(const std::string& payload) {
        auto result = _publisher->publish(PublishRequest{
            .topic = _topic,
            .key = "same-key",
            .payload = payload,
            .metadata = MessageMetadata{.eventId = shared::models::OutboxEventId::create(nextEventId()).value(),
                                        .eventType = "TestEvent"},
        });

        ASSERT_EQ(result.wait_for(5s), std::future_status::ready);
        ASSERT_TRUE(result.get().has_value());
    }

    std::unique_ptr<KafkaEventConsumer> startConsumer(IEventHandler& handler) {
        auto consumer = std::make_unique<KafkaEventConsumer>(
            KafkaConsumerConfiguration{.brokers = _brokers, .groupId = _groupId, .topic = _topic});
        EXPECT_TRUE(consumer->start(handler).has_value());
        return consumer;
    }

    std::string _brokers;
    std::string _topic;
    std::string _groupId;
    std::unique_ptr<KafkaEventPublisher> _publisher;

   private:
    std::string nextEventId() {
        char buffer[37];
        std::snprintf(buffer, sizeof(buffer), "00000000-0000-0000-0000-%012u", ++_eventCounter);
        return buffer;
    }

    unsigned _eventCounter = 0;
};

// Сбойное сообщение должно прийти повторно и раньше последующих
TEST_F(KafkaEventConsumerIntegrationTest, FailedMessageIsRedeliveredBeforeLaterMessages) {
    publish("m1");
    publish("m2");
    publish("m3");

    ScriptedHandler handler({{"m1", 1}});  // m1 проваливается один раз
    auto consumer = startConsumer(handler);

    const bool done = handler.waitFor([](const auto& succeeded, const auto&) { return succeeded.size() == 3; }, 30s);
    consumer->stop();

    ASSERT_TRUE(done) << "не все сообщения были обработаны: сбойное потеряно";
    EXPECT_EQ(handler.succeeded(), (std::vector<std::string>{"m1", "m2", "m3"}));
    EXPECT_EQ(handler.attempts("m1"), 2);
}

// Сбойное сообщение нельзя "перепрыгнуть" коммитом более позднего: после рестарта оно должно прийти снова
TEST_F(KafkaEventConsumerIntegrationTest, FailedMessageIsNotSkippedByLaterCommitAfterRestart) {
    publish("m1");
    publish("m2");

    {
        ScriptedHandler brokenHandler({{"m1", -1}});  // m1 проваливается всегда
        auto consumer = startConsumer(brokenHandler);

        ASSERT_TRUE(brokenHandler.waitFor(
            [](const auto&, const auto& attempts) {
                const auto it = attempts.find("m1");
                return it != attempts.end() && it->second >= 2;
            },
            30s)) << "m1 не был повторён внутри живого консьюмера";

        consumer->stop();
        EXPECT_TRUE(brokenHandler.succeeded().empty()) << "m2 обработан в обход сбойного m1";
    }  // консьюмер уничтожен => вышел из группы

    ScriptedHandler healthyHandler;
    auto consumer = startConsumer(healthyHandler);

    const bool done =
        healthyHandler.waitFor([](const auto& succeeded, const auto&) { return succeeded.size() == 2; }, 30s);
    consumer->stop();

    ASSERT_TRUE(done);
    EXPECT_EQ(healthyHandler.succeeded(), (std::vector<std::string>{"m1", "m2"}));
}

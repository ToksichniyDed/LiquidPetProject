//
// Created by DED on 19.09.2026.
//

// Интеграционный тест: требует поднятой Kafka (docker compose up -d --wait kafka)

#include <gtest/gtest.h>
#include <logging/Logger.h>
#include <messaging/PublisherDeadLetterSink.h>
#include <messaging/consumer/KafkaEventConsumer.h>
#include <messaging/producer/KafkaEventPublisher.h>

#include <algorithm>
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
// failures: сколько первых попыток провалить для payload (-1 = всегда)
class ScriptedHandler : public IEventHandler {
public:
    explicit ScriptedHandler(std::map<std::string, int> failures = {}) : _failures(std::move(failures)) {
    }

    bool handle(const std::string& payload, const MessageMetadata&) override {
        std::lock_guard lock(_mutex);
        ++_attempts[payload];

        if (auto it = _failures.find(payload); it != _failures.end() && it->second != 0) {
            if (it->second > 0)
                --it->second;
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

constexpr RetryPolicy FAST_RETRY{.maxAttempts = 3, .initialBackoff = 10ms, .multiplier = 1.0, .maxBackoff = 10ms};

bool contains(const std::vector<std::string>& values, const std::string& value) {
    return std::ranges::find(values, value) != values.end();
}
} // namespace

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

        _publisher = std::make_shared<KafkaEventPublisher>(_brokers);
        _deadLetterSink = std::make_shared<PublisherDeadLetterSink>(_publisher);
    }

    // Один и тот же ключ => одна партиция
    void publish(const std::string& payload, const std::string& topic) {
        auto result = _publisher->publish(PublishRequest{
            .topic = topic,
            .key = "same-key",
            .payload = payload,
            .metadata = MessageMetadata{.eventId = shared::models::OutboxEventId::create(nextEventId()).value(),
                                        .eventType = "TestEvent"},
        });

        ASSERT_EQ(result.wait_for(5s), std::future_status::ready);
        ASSERT_TRUE(result.get().has_value());
    }

    void publish(const std::string& payload) { publish(payload, _topic); }

    std::unique_ptr<KafkaEventConsumer> startConsumerOn(const std::string& topic, const std::string& groupId,
                                                        IEventHandler& handler, RetryPolicy retry) {
        auto consumer = std::make_unique<KafkaEventConsumer>(
            KafkaConsumerConfiguration{.brokers = _brokers, .groupId = groupId, .topic = topic}, _deadLetterSink,
            PipelineConfiguration{.workerThreads = 4, .queueCapacity = 16, .retry = retry});
        EXPECT_TRUE(consumer->start(handler).has_value());
        return consumer;
    }

    std::unique_ptr<KafkaEventConsumer> startConsumer(IEventHandler& handler, RetryPolicy retry = FAST_RETRY) {
        return startConsumerOn(_topic, _groupId, handler, retry);
    }

    std::string _brokers;
    std::string _topic;
    std::string _groupId;
    std::shared_ptr<KafkaEventPublisher> _publisher;
    std::shared_ptr<PublisherDeadLetterSink> _deadLetterSink;

private:
    std::string nextEventId() {
        char buffer[37];
        std::snprintf(buffer, sizeof(buffer), "00000000-0000-0000-0000-%012u", ++_eventCounter);
        return buffer;
    }

    unsigned _eventCounter = 0;
};

// Сбойная запись повторяется точечно, остальные не перечитываются
TEST_F(KafkaEventConsumerIntegrationTest, FailedMessageIsRetriedInPlace) {
    publish("m1");
    publish("m2");
    publish("m3");

    ScriptedHandler handler({{"m1", 1}}); // m1 проваливается один раз
    auto consumer = startConsumer(handler);

    const bool done = handler.waitFor([](const auto& succeeded, const auto&) { return succeeded.size() == 3; }, 30s);
    consumer->stop();

    ASSERT_TRUE(done) << "не все сообщения были обработаны";
    auto succeeded = handler.succeeded();
    std::ranges::sort(succeeded);
    EXPECT_EQ(succeeded, (std::vector<std::string>{"m1", "m2", "m3"}));
    EXPECT_EQ(handler.attempts("m1"), 2);
    EXPECT_EQ(handler.attempts("m2"), 1);
    EXPECT_EQ(handler.attempts("m3"), 1);
}

// Ядовитое сообщение: после исчерпания попыток уходит в DLQ, остальные не блокируются
TEST_F(KafkaEventConsumerIntegrationTest, PoisonMessageGoesToDeadLetterTopic) {
    const auto dlqTopic = _topic + ".dlq";
    publish("dlq-init", dlqTopic); // создаёт DLQ-топик заранее, чтобы подписка не ждала обновления метаданных
    publish("m1");
    publish("m2");

    ScriptedHandler dlqHandler;
    auto dlqConsumer = startConsumerOn(dlqTopic, _groupId + "-dlq", dlqHandler, FAST_RETRY);

    ScriptedHandler handler({{"m1", -1}}); // m1 проваливается всегда
    auto consumer = startConsumer(handler, RetryPolicy{.maxAttempts = 2,
                                                       .initialBackoff = 10ms,
                                                       .multiplier = 1.0,
                                                       .maxBackoff = 10ms});

    const bool inDlq = dlqHandler.waitFor(
        [](const auto& succeeded, const auto&) { return contains(succeeded, "m1"); }, 30s);
    const bool othersDone =
        handler.waitFor([](const auto& succeeded, const auto&) { return contains(succeeded, "m2"); }, 30s);
    consumer->stop();
    dlqConsumer->stop();

    EXPECT_TRUE(inDlq) << "m1 не появилось в DLQ";
    EXPECT_TRUE(othersDone) << "m2 заблокирован ядовитым m1";
    EXPECT_EQ(handler.attempts("m1"), 2);
}

// Зависшую запись нельзя "перепрыгнуть" коммитом более поздней: после рестарта она должна прийти снова
TEST_F(KafkaEventConsumerIntegrationTest, StuckMessageIsNotSkippedByLaterCommitAfterRestart) {
    publish("m1");
    publish("m2");

    {
        ScriptedHandler brokenHandler({{"m1", -1}});
        // Попыток "бесконечно много": m1 остаётся в обработке и держит коммит
        auto consumer = startConsumer(brokenHandler, RetryPolicy{.maxAttempts = 100000,
                                                                 .initialBackoff = 5ms,
                                                                 .multiplier = 1.0,
                                                                 .maxBackoff = 5ms});

        ASSERT_TRUE(brokenHandler.waitFor(
            [](const auto& succeeded, const auto& attempts) {
            const auto it = attempts.find("m1");
            return contains(succeeded, "m2") && it != attempts.end() && it->second >= 2;
            },
            30s)) << "m2 не обработан или m1 не повторялся";

        consumer->stop();
    } // консьюмер уничтожен => вышел из группы

    ScriptedHandler healthyHandler;
    auto consumer = startConsumer(healthyHandler);

    const bool gotM1 =
        healthyHandler.waitFor([](const auto& succeeded, const auto&) { return contains(succeeded, "m1"); }, 30s);
    consumer->stop();

    ASSERT_TRUE(gotM1) << "m1 потерян: коммит перепрыгнул зависшую запись";
}

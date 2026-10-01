//
// Created by DED on 02.10.2026.
//

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "RetryPolicy.h"

using namespace shared::messaging;
using namespace std::chrono_literals;

namespace {
constexpr RetryPolicy POLICY{.maxAttempts = 5, .initialBackoff = 200ms, .multiplier = 2.0, .maxBackoff = 10000ms};
}

TEST(RetryPolicyTest, BackoffGrowsExponentially) {
    EXPECT_EQ(POLICY.delayAfterAttempt(1), 200ms);
    EXPECT_EQ(POLICY.delayAfterAttempt(2), 400ms);
    EXPECT_EQ(POLICY.delayAfterAttempt(3), 800ms);
}

TEST(RetryPolicyTest, BackoffIsCappedByMaxBackoff) {
    EXPECT_EQ(POLICY.delayAfterAttempt(10), 10000ms);
}

TEST(RetryPolicyTest, BackoffSurvivesHugeAttemptNumbers) {
    // 2^1000 не помещается в double-экспоненту нормально - результат всё равно должен быть capped
    EXPECT_EQ(POLICY.delayAfterAttempt(5000), 10000ms);
}

TEST(RetryPolicyTest, NonPositiveAttemptIsTreatedAsFirst) {
    EXPECT_EQ(POLICY.delayAfterAttempt(0), 200ms);
    EXPECT_EQ(POLICY.delayAfterAttempt(-3), 200ms);
}

TEST(RetryPolicyTest, MaxAttemptsBelowOneMeansSingleAttempt) {
    EXPECT_EQ(RetryPolicy{.maxAttempts = 0}.effectiveMaxAttempts(), 1);
    EXPECT_EQ(RetryPolicy{.maxAttempts = -5}.effectiveMaxAttempts(), 1);
    EXPECT_EQ(RetryPolicy{.maxAttempts = 7}.effectiveMaxAttempts(), 7);
}

TEST(SleepUnlessStoppedTest, ReturnsTrueAfterFullDuration) {
    std::stop_source source;

    EXPECT_TRUE(sleepUnlessStopped(source.get_token(), 5ms));
}

TEST(SleepUnlessStoppedTest, ReturnsFalseImmediatelyIfAlreadyStopped) {
    std::stop_source source;
    source.request_stop();

    const auto started = std::chrono::steady_clock::now();
    EXPECT_FALSE(sleepUnlessStopped(source.get_token(), 10s));
    EXPECT_LT(std::chrono::steady_clock::now() - started, 1s);
}

TEST(SleepUnlessStoppedTest, WakesUpWhenStopIsRequestedDuringSleep) {
    std::stop_source source;
    std::jthread stopper([&source] {
        std::this_thread::sleep_for(20ms);
        source.request_stop();
    });

    const auto started = std::chrono::steady_clock::now();
    EXPECT_FALSE(sleepUnlessStopped(source.get_token(), 10s));
    EXPECT_LT(std::chrono::steady_clock::now() - started, 5s);
}

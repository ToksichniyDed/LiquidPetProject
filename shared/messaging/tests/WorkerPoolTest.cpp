//
// Created by DED on 02.10.2026.
//

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <latch>
#include <thread>

#include "WorkerPool.h"

using namespace shared::messaging;
using namespace std::chrono_literals;

namespace {
// Задача, которая занимает поток, пока её не отпустят или не остановят пул
WorkerPool::Task blockUntil(std::shared_future<void> release, std::promise<void>* started = nullptr) {
    return [release = std::move(release), started](const std::stop_token& stopToken) {
        if (started)
            started->set_value();
        while (!stopToken.stop_requested() && release.wait_for(1ms) != std::future_status::ready) {
        }
    };
}
}  // namespace

TEST(WorkerPoolTest, RunsAllSubmittedTasks) {
    constexpr int COUNT = 50;
    WorkerPool pool(4, COUNT);
    std::latch done(COUNT);
    std::atomic<int> executed{0};

    for (int index = 0; index < COUNT; ++index) {
        ASSERT_TRUE(pool.trySubmit([&](const std::stop_token&) {
            ++executed;
            done.count_down();
        }));
    }

    done.wait();
    EXPECT_EQ(executed, COUNT);
}

TEST(WorkerPoolTest, RejectsTasksWhenQueueIsFull) {
    WorkerPool pool(1, 2);
    std::promise<void> release;
    std::promise<void> started;

    ASSERT_TRUE(pool.trySubmit(blockUntil(release.get_future().share(), &started)));
    started.get_future().wait();  // поток занят, очередь пуста

    EXPECT_TRUE(pool.trySubmit([](const std::stop_token&) {}));
    EXPECT_TRUE(pool.trySubmit([](const std::stop_token&) {}));
    EXPECT_FALSE(pool.trySubmit([](const std::stop_token&) {}));  // ёмкость 2 исчерпана
    EXPECT_EQ(pool.pendingCount(), 2);

    release.set_value();
}

TEST(WorkerPoolTest, RunsTasksInParallel) {
    constexpr int THREADS = 3;
    WorkerPool pool(THREADS, THREADS);
    std::atomic<int> running{0};
    std::atomic<bool> sawAllRunning{false};
    std::latch done(THREADS);

    for (int index = 0; index < THREADS; ++index) {
        ASSERT_TRUE(pool.trySubmit([&](const std::stop_token&) {
            ++running;
            const auto deadline = std::chrono::steady_clock::now() + 2s;
            while (running < THREADS && std::chrono::steady_clock::now() < deadline)
                std::this_thread::yield();
            if (running == THREADS)
                sawAllRunning = true;
            done.count_down();
        }));
    }

    done.wait();
    EXPECT_TRUE(sawAllRunning);
}

TEST(WorkerPoolTest, StopDropsPendingTasksAndInterruptsRunningOnes) {
    WorkerPool pool(1, 10);
    std::promise<void> release;  // намеренно не отпускаем: задачу остановит stop()
    std::promise<void> started;
    std::atomic<int> executedPending{0};

    ASSERT_TRUE(pool.trySubmit(blockUntil(release.get_future().share(), &started)));
    started.get_future().wait();
    for (int index = 0; index < 5; ++index)
        ASSERT_TRUE(pool.trySubmit([&](const std::stop_token&) { ++executedPending; }));

    pool.stop();

    EXPECT_EQ(executedPending, 0);
    EXPECT_EQ(pool.pendingCount(), 0);
}

TEST(WorkerPoolTest, SubmitAfterStopIsRejected) {
    WorkerPool pool(2, 10);
    pool.stop();

    EXPECT_FALSE(pool.trySubmit([](const std::stop_token&) {}));
}

TEST(WorkerPoolTest, StopIsIdempotent) {
    WorkerPool pool(2, 10);

    pool.stop();
    pool.stop();
}

TEST(WorkerPoolTest, ThrowingTaskDoesNotKillTheWorker) {
    WorkerPool pool(1, 10);
    std::promise<void> ran;

    ASSERT_TRUE(pool.trySubmit([](const std::stop_token&) { throw std::runtime_error("boom"); }));
    ASSERT_TRUE(pool.trySubmit([&](const std::stop_token&) { ran.set_value(); }));

    EXPECT_EQ(ran.get_future().wait_for(2s), std::future_status::ready);
}

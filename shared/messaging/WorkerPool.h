//
// Created by DED on 01.10.2026.
//

#ifndef LIQUIDPETPROJECT_WORKERPOOL_H
#define LIQUIDPETPROJECT_WORKERPOOL_H

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <stop_token>
#include <thread>
#include <vector>

#include <logging/Logger.h>

namespace shared::messaging {

// Пул потоков с ограниченной очередью. Переполнение - сигнал читателю притормозить (backpressure),
// поэтому trySubmit не блокируется, а возвращает false.
// Порядок между задачами не гарантируется
class WorkerPool {
   public:
    using Task = std::function<void(std::stop_token)>;

    WorkerPool(std::size_t threadCount, std::size_t queueCapacity)
        : _capacity(std::max<std::size_t>(1, queueCapacity)) {
        const auto count = std::max<std::size_t>(1, threadCount);
        _threads.reserve(count);
        for (std::size_t index = 0; index < count; ++index)
            _threads.emplace_back([this](std::stop_token stopToken) { workerLoop(stopToken); });
    }

    ~WorkerPool() { stop(); }

    WorkerPool(const WorkerPool&) = delete;
    WorkerPool& operator=(const WorkerPool&) = delete;

    // false - очередь полна или пул остановлен: задача НЕ принята
    [[nodiscard]] bool trySubmit(Task task) {
        {
            std::scoped_lock lock(_mutex);
            if (_stopped || _queue.size() >= _capacity)
                return false;
            _queue.push_back(std::move(task));
        }
        _condition.notify_one();
        return true;
    }

    // Принятые, но ещё не начатые задачи отбрасываются: их записи не завершены,
    // значит не закоммичены и будут доставлены заново. Выполняющиеся задачи получают stop_token.
    // Идемпотентен; из нескольких потоков одновременно не вызывать.
    void stop() {
        {
            std::scoped_lock lock(_mutex);
            _stopped = true;
            _queue.clear();
        }
        for (auto& thread : _threads)
            thread.request_stop();
        _threads.clear();  // jthread в деструкторе дожидается завершения
    }

    [[nodiscard]] std::size_t pendingCount() const {
        std::scoped_lock lock(_mutex);
        return _queue.size();
    }

   private:
    void workerLoop(const std::stop_token& stopToken) {
        while (true) {
            Task task;
            {
                std::unique_lock lock(_mutex);
                _condition.wait(lock, stopToken, [this] { return !_queue.empty(); });
                if (stopToken.stop_requested())
                    return;

                task = std::move(_queue.front());
                _queue.pop_front();
            }

            try {
                task(stopToken);
            } catch (const std::exception& e) {
                SPDLOG_LOGGER_ERROR(shared::logger::get("WorkerPool"), "Task threw: {}", e.what());
            } catch (...) {
                SPDLOG_LOGGER_ERROR(shared::logger::get("WorkerPool"), "Task threw unknown exception");
            }
        }
    }

   private:
    mutable std::mutex _mutex;
    std::condition_variable_any _condition;
    std::deque<Task> _queue;
    std::size_t _capacity;
    bool _stopped = false;
    std::vector<std::jthread> _threads;
};

}  // namespace shared::messaging

#endif  // LIQUIDPETPROJECT_WORKERPOOL_H

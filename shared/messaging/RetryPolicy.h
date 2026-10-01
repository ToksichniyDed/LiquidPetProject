//
// Created by DED on 01.10.2026.
//

#ifndef LIQUIDPETPROJECT_RETRYPOLICY_H
#define LIQUIDPETPROJECT_RETRYPOLICY_H

#include <algorithm>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <stop_token>

namespace shared::messaging {

// Экспоненциальная пауза: initialBackoff * multiplier^(attempt-1), но не больше maxBackoff.
// maxAttempts < 1 трактуется как 1: обработчик всегда вызывается хотя бы раз.
struct RetryPolicy {
    int maxAttempts = 5;
    std::chrono::milliseconds initialBackoff{200};
    double multiplier = 2.0;
    std::chrono::milliseconds maxBackoff{10000};

    // Пауза после неудачной попытки номер attempt (нумерация с 1)
    [[nodiscard]] std::chrono::milliseconds delayAfterAttempt(int attempt) const {
        const auto exponent = std::max(0, attempt - 1);
        const double raw = static_cast<double>(initialBackoff.count()) * std::pow(multiplier, exponent);
        // Сначала ограничиваем в double
        const double capped = std::min(raw, static_cast<double>(maxBackoff.count()));
        return std::chrono::milliseconds{static_cast<std::chrono::milliseconds::rep>(capped)};
    }

    [[nodiscard]] int effectiveMaxAttempts() const { return std::max(1, maxAttempts); }
};

// Спит duration, но просыпается сразу, если запрошена остановка.
// true  - паузу отсидели целиком
// false - прервали остановкой
inline bool sleepUnlessStopped(const std::stop_token& stopToken, std::chrono::milliseconds duration) {
    std::mutex mutex;
    std::condition_variable_any condition;
    std::unique_lock lock(mutex);

    // Предикат всегда false: выходим только по таймауту или по stop_token
    condition.wait_for(lock, stopToken, duration, [] { return false; });
    return !stopToken.stop_requested();
}

}

#endif  // LIQUIDPETPROJECT_RETRYPOLICY_H

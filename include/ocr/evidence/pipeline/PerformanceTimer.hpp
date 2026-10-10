#pragma once

#include <chrono>
#include <map>
#include <string>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

/**
 * @brief High-resolution microsecond timer for stage-by-stage pipeline performance benchmarking.
 */
class PerformanceTimer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    void startStage(const std::string& stageName) {
        currentStage_ = stageName;
        stageStart_ = Clock::now();
    }

    void stopStage() {
        if (!currentStage_.empty()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                Clock::now() - stageStart_
            ).count();
            durationsUs_[currentStage_] += elapsed;
            currentStage_.clear();
        }
    }

    [[nodiscard]] double getStageDurationMs(const std::string& stageName) const noexcept {
        auto it = durationsUs_.find(stageName);
        if (it != durationsUs_.end()) {
            return static_cast<double>(it->second) / 1000.0;
        }
        return 0.0;
    }

    [[nodiscard]] double getTotalDurationMs() const noexcept {
        long long totalUs = 0;
        for (const auto& [k, v] : durationsUs_) {
            totalUs += v;
        }
        return static_cast<double>(totalUs) / 1000.0;
    }

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json j;
        for (const auto& [k, v] : durationsUs_) {
            j[k + "_ms"] = static_cast<double>(v) / 1000.0;
        }
        j["total_execution_time_ms"] = getTotalDurationMs();
        return j;
    }

private:
    std::string currentStage_;
    TimePoint stageStart_;
    std::map<std::string, long long> durationsUs_;
};

} // namespace ocr::evidence

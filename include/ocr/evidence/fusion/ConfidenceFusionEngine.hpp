#pragma once

#include <algorithm>
#include <string>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

struct FusionWeights {
    float ocr = 0.20f;
    float llm = 0.60f;
    float dictionary = 0.10f;
    float context = 0.10f;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"ocr", ocr},
            {"llm", llm},
            {"dictionary", dictionary},
            {"context", context}
        };
    }

    [[nodiscard]] static FusionWeights fromJson(const nlohmann::json& j) {
        FusionWeights w;
        if (!j.is_object()) return w;
        w.ocr = j.value("ocr", 0.20f);
        w.llm = j.value("llm", 0.60f);
        w.dictionary = j.value("dictionary", 0.10f);
        w.context = j.value("context", 0.10f);
        return w;
    }
};

struct FusionPolicy {
    std::string version = "fusion-v1";
    FusionWeights weights;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"version", version},
            {"weights", weights.toJson()}
        };
    }

    [[nodiscard]] static FusionPolicy fromJson(const nlohmann::json& j) {
        FusionPolicy p;
        if (!j.is_object()) return p;
        p.version = j.value("version", "fusion-v1");
        if (j.contains("weights") && j["weights"].is_object()) {
            p.weights = FusionWeights::fromJson(j["weights"]);
        }
        return p;
    }
};

/**
 * @brief Configurable and versioned confidence fusion engine for LLM patches.
 */
class ConfidenceFusionEngine {
public:
    explicit ConfidenceFusionEngine(FusionPolicy policy = {})
        : policy_(std::move(policy)) {}

    [[nodiscard]] float fuseLlmPatchConfidence(
        float ocrConfidence,
        float llmConfidence,
        float dictionaryScore = 1.0f,
        float contextScore = 1.0f
    ) const noexcept {
        float score = (policy_.weights.ocr * std::clamp(ocrConfidence, 0.0f, 1.0f)) +
                      (policy_.weights.llm * std::clamp(llmConfidence, 0.0f, 1.0f)) +
                      (policy_.weights.dictionary * std::clamp(dictionaryScore, 0.0f, 1.0f)) +
                      (policy_.weights.context * std::clamp(contextScore, 0.0f, 1.0f));
        return std::clamp(score, 0.0f, 1.0f);
    }

    [[nodiscard]] const FusionPolicy& policy() const noexcept {
        return policy_;
    }

    void setPolicy(FusionPolicy policy) noexcept {
        policy_ = std::move(policy);
    }

private:
    FusionPolicy policy_;
};

} // namespace ocr::evidence

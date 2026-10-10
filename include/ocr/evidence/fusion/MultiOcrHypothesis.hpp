#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

/**
 * @brief Represents an OCR hypothesis from an independent OCR pass or engine.
 */
struct OcrHypothesis {
    std::string source; // "ocr_primary", "ocr_secondary", "ocr_tertiary"
    std::string value;
    float confidence = 1.0f;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"source", source},
            {"value", value},
            {"confidence", confidence}
        };
    }

    static OcrHypothesis fromJson(const nlohmann::json& j) {
        OcrHypothesis h;
        if (j.is_object()) {
            h.source = j.value("source", "ocr_primary");
            h.value = j.value("value", "");
            h.confidence = j.value("confidence", 1.0f);
        }
        return h;
    }
};

/**
 * @brief Collection of multiple OCR hypotheses for a single evidence element.
 */
struct MultiOcrInput {
    std::vector<OcrHypothesis> sources;

    [[nodiscard]] bool empty() const noexcept {
        return sources.empty();
    }
};

} // namespace ocr::evidence

#pragma once

#include <ocr/evidence/models/EvidenceSource.hpp>
#include <ocr/evidence/models/Fates.hpp>

#include <string>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

/**
 * @brief Represents an individual OCR token with confidence and geometry.
 */
struct EvidenceToken {
    std::string id;
    std::string text;
    float confidence = 1.0f;
    BoundingBox bbox;
    int page = 1;
    EvidenceFate fate = EvidenceFate::Preserved;

    [[nodiscard]] bool empty() const noexcept {
        return text.empty();
    }

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"id", id},
            {"text", text},
            {"confidence", confidence},
            {"bbox", bbox.toJson()},
            {"page", page},
            {"fate", fateToString(fate)}
        };
    }
};

/**
 * @brief Summary statistics of confidence across a collection of tokens.
 */
struct ConfidenceSummary {
    float mean = 1.0f;
    float minimum = 1.0f;
    float maximum = 1.0f;
    int tokenCount = 0;
    int lowConfidenceCount = 0;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"mean", mean},
            {"minimum", minimum},
            {"maximum", maximum},
            {"token_count", tokenCount},
            {"low_confidence_token_count", lowConfidenceCount}
        };
    }

    static ConfidenceSummary fromJson(const nlohmann::json& j) {
        ConfidenceSummary cs;
        if (j.is_object()) {
            cs.mean = j.value("mean", 1.0f);
            cs.minimum = j.value("minimum", 1.0f);
            cs.maximum = j.value("maximum", 1.0f);
            cs.tokenCount = j.value("token_count", 0);
            cs.lowConfidenceCount = j.value("low_confidence_token_count", 0);
        }
        return cs;
    }
};

} // namespace ocr::evidence

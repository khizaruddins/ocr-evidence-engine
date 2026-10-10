#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

/**
 * @brief Structured problem diagnosis explaining why a field requires resolution or failed resolution.
 */
struct ResolutionProblem {
    std::string type;                   ///< e.g. AMBIGUOUS_FIELD_VALUE, CONFLICTING_EVIDENCE, FIELD_MAPPING_AMBIGUITY, MALFORMED_OCR, TABLE_COLUMN_AMBIGUITY, NORMALIZATION_AMBIGUITY, INSUFFICIENT_EVIDENCE
    std::string description;            ///< Human-readable description of the problem
    std::string reason;                 ///< Detailed reason why deterministic resolution cannot proceed
    std::string deterministicAttempt;   ///< Description of the deterministic logic attempted
    std::string ambiguity;              ///< Specific ambiguity or competing interpretations

    [[nodiscard]] bool empty() const noexcept {
        return type.empty();
    }

    [[nodiscard]] nlohmann::json toJson() const {
        if (empty()) return nlohmann::json::object();
        return nlohmann::json{
            {"type", type},
            {"description", description},
            {"reason", reason},
            {"deterministic_attempt", deterministicAttempt},
            {"ambiguity", ambiguity}
        };
    }

    [[nodiscard]] static ResolutionProblem fromJson(const nlohmann::json& j) {
        ResolutionProblem p;
        if (!j.is_object()) return p;
        p.type = j.value("type", "");
        p.description = j.value("description", "");
        p.reason = j.value("reason", "");
        p.deterministicAttempt = j.value("deterministic_attempt", "");
        p.ambiguity = j.value("ambiguity", "");
        return p;
    }
};

} // namespace ocr::evidence

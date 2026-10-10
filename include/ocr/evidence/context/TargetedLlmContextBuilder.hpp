#pragma once

#include <edmo/evidence/models/EvidenceDocument.hpp>
#include <edmo/evidence/models/EvidenceField.hpp>

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace edmo::evidence {

/**
 * @brief Context expansion level for targeted evidence retrieval.
 */
enum class ContextLevel {
    Targeted,   ///< Minimal immediate structural and spatial context.
    Expanded    ///< Broader section, neighboring rows/tables, and cross-page candidate context.
};

[[nodiscard]] constexpr std::string_view contextLevelToString(ContextLevel cl) noexcept {
    return cl == ContextLevel::Expanded ? "EXPANDED" : "TARGETED";
}

/**
 * @brief Builds targeted LLM context and resolution request payloads with rich EDR semantics.
 */
class TargetedLlmContextBuilder {
public:
    /**
     * @brief Build targeted LLM context JSON object for unresolved fields.
     */
    [[nodiscard]] static nlohmann::json buildContext(
        const EvidenceDocument& doc,
        const std::vector<EvidenceField>& fields,
        ContextLevel contextLevel = ContextLevel::Targeted
    );

    [[nodiscard]] static nlohmann::json buildContext(
        const EvidenceDocument& doc,
        std::vector<EvidenceField>& fields,
        ContextLevel contextLevel = ContextLevel::Targeted
    );

    /**
     * @brief Build dedicated LLM resolution request JSON object (schema: edmo-llm-resolution-request-0.1).
     */
    [[nodiscard]] static nlohmann::json buildResolutionRequest(
        const EvidenceDocument& doc,
        const std::vector<EvidenceField>& fields,
        ContextLevel contextLevel = ContextLevel::Targeted
    );

    [[nodiscard]] static nlohmann::json buildResolutionRequest(
        const EvidenceDocument& doc,
        std::vector<EvidenceField>& fields,
        ContextLevel contextLevel = ContextLevel::Targeted
    );

    /**
     * @brief Estimate token count for the serialized JSON payload.
     */
    [[nodiscard]] static int estimateTokens(const nlohmann::json& contextJson) noexcept;
};

} // namespace edmo::evidence

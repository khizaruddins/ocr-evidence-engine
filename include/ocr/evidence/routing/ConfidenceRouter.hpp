#pragma once

#include <edmo/evidence/confidence/ConfidenceComparator.hpp>
#include <edmo/evidence/models/EvidenceDocument.hpp>
#include <edmo/evidence/models/EvidenceField.hpp>
#include <edmo/evidence/routing/RouterConfig.hpp>

#include <vector>
#include <string>
#include <nlohmann/json.hpp>

namespace edmo::evidence {

struct LlmRoutingDecision {
    bool eligible = false;
    std::string reasonCode;
    std::string field;
    float confidence = 0.0f;
    float threshold = 0.90f;
    bool semanticValid = true;
    bool deterministicallyResolved = false;
    bool evidenceSufficient = true;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"eligible", eligible},
            {"reason_code", reasonCode},
            {"field", field},
            {"confidence", confidence},
            {"threshold", threshold},
            {"semantic_valid", semanticValid},
            {"deterministically_resolved", deterministicallyResolved},
            {"evidence_sufficient", evidenceSufficient}
        };
    }
};

struct RoutingDecision {
    bool isTrusted = true;
    bool llmRequired = false;
    bool llmEligible = true;
    EvidenceFate fate = EvidenceFate::Preserved;
    ResolutionStatus resolutionStatus = ResolutionStatus::AiUnresolved;
    std::string reason;
    LlmRoutingDecision llmDecision;
};

struct RoutingStats {
    float threshold = 0.90f;
    float epsilon = 0.001f;
    int totalCandidates = 0;
    int structuralItems = 0;
    int validFields = 0;
    int highConfidence = 0;
    int deterministicallyResolved = 0;
    int structural = 0;
    int placeholders = 0;
    int noise = 0;
    int invalid = 0;
    int insufficientEvidence = 0;
    int llmEligible = 0;
    int llmRequired = 0;
    int llmTasksCreated = 0;
    int llmRejected = 0;
    int placeholderRejections = 0;
    int structuralRejections = 0;
    int invalidFieldRejections = 0;
    int duplicateTasksRemoved = 0;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"threshold", threshold},
            {"epsilon", epsilon},
            {"total_candidates", totalCandidates},
            {"high_confidence", highConfidence},
            {"deterministically_resolved", deterministicallyResolved},
            {"structural", structural},
            {"placeholders", placeholders},
            {"invalid", invalid},
            {"insufficient_evidence", insufficientEvidence},
            {"llm_eligible", llmEligible},
            {"llm_tasks_created", llmTasksCreated > 0 ? llmTasksCreated : llmRequired},
            {"structural_items", structuralItems},
            {"valid_fields", validFields},
            {"llm_required", llmRequired},
            {"llm_rejected", llmRejected},
            {"quality", {
                {"placeholder_rejections", placeholderRejections},
                {"structural_rejections", structuralRejections},
                {"invalid_field_rejections", invalidFieldRejections},
                {"duplicate_tasks_removed", duplicateTasksRemoved}
            }}
        };
    }
};

/**
 * @brief Routes evidence items to TRUST or TARGETED_LLM based on fused confidence scores and structural validity.
 */
class ConfidenceRouter {
public:
    explicit ConfidenceRouter(RouterConfig config = {})
        : config_(config) {}

    /**
     * @brief Route a single evidence field to trust or LLM.
     */
    [[nodiscard]] RoutingDecision routeField(
        const EvidenceField& field
    ) const noexcept;

    /**
     * @brief Route all fields in a collection, enforce deduplication and budget, and collect metrics.
     */
    [[nodiscard]] RoutingStats routeFields(std::vector<EvidenceField>& fields) const noexcept;
    [[nodiscard]] RoutingStats routeFields(std::vector<EvidenceField>& fields, const EvidenceDocument& doc) const noexcept;

    [[nodiscard]] const RouterConfig& config() const noexcept {
        return config_;
    }

    void setConfig(const RouterConfig& config) noexcept {
        config_ = config;
    }

private:
    RouterConfig config_;
};

} // namespace edmo::evidence

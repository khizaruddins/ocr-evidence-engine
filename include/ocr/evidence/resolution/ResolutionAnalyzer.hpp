#pragma once

#include <edmo/evidence/models/EvidenceDocument.hpp>
#include <edmo/evidence/models/EvidenceField.hpp>
#include <edmo/evidence/resolution/ResolutionProblem.hpp>
#include <edmo/evidence/resolution/ResolutionStrategy.hpp>
#include <edmo/evidence/routing/RouterConfig.hpp>

#include <vector>
#include <string>

namespace edmo::evidence {

/**
 * @brief Analysis result produced by ResolutionAnalyzer for a single field.
 */
struct FieldResolutionAnalysis {
    ResolutionStatus status = ResolutionStatus::AiUnresolved;
    ResolutionStrategy strategy = ResolutionStrategy::None;
    bool isTrusted = true;
    bool llmRequired = false;
    bool isAmbiguous = false;
    std::string reasonCode;
    std::string reasonMessage;
    ResolutionProblem problem;
};

/**
 * @brief High-level resolution analysis summary for an entire document.
 */
struct DocumentResolutionSummary {
    int totalFields = 0;
    int deterministicCount = 0;
    int ocrConfidentCount = 0;
    int normalizedCount = 0;
    int llmRequiredCount = 0;
    int resolvedCount = 0;
    int aiUnresolvedCount = 0;
    bool hasMandatoryUnresolved = false;
    bool fastPathEligible = false;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"total_fields", totalFields},
            {"deterministic", deterministicCount},
            {"ocr_confident", ocrConfidentCount},
            {"normalized", normalizedCount},
            {"llm_required", llmRequiredCount},
            {"resolved", resolvedCount},
            {"ai_unresolved", aiUnresolvedCount},
            {"has_mandatory_unresolved", hasMandatoryUnresolved},
            {"fast_path_eligible", fastPathEligible}
        };
    }
};

/**
 * @brief Semantic Resolution Analyzer for evidence-driven field decisioning.
 *
 * Evaluates semantic signals across EDR blocks, label-value relationships, table structures,
 * duplicate candidates, and schema validity to determine whether a field can be deterministically
 * resolved or requires targeted AI resolution with explicit problem diagnosis.
 */
class ResolutionAnalyzer {
public:
    explicit ResolutionAnalyzer(RouterConfig config = {})
        : config_(std::move(config)) {}

    /**
     * @brief Analyze a single evidence field in context of the document.
     */
    [[nodiscard]] FieldResolutionAnalysis analyzeField(
        const EvidenceField& field,
        const EvidenceDocument& doc,
        const std::vector<EvidenceField>& allFields
    ) const noexcept;

    /**
     * @brief Analyze and annotate all fields in a document.
     */
    DocumentResolutionSummary analyzeDocument(
        std::vector<EvidenceField>& fields,
        const EvidenceDocument& doc
    ) const noexcept;

    [[nodiscard]] const RouterConfig& config() const noexcept {
        return config_;
    }

    void setConfig(const RouterConfig& config) noexcept {
        config_ = config;
    }

    [[nodiscard]] float calculateStructuralCreditsConfidence(
        const EvidenceField& field,
        const EvidenceDocument& doc,
        const std::vector<EvidenceField>& allFields,
        std::string* outReason = nullptr
    ) const noexcept;

    [[nodiscard]] bool isStrongStructuralCreditsEvidence(
        const EvidenceField& field,
        const EvidenceDocument& doc,
        const std::vector<EvidenceField>& allFields,
        float& outConfidence
    ) const noexcept;

private:
    RouterConfig config_;

    [[nodiscard]] bool isSupportedLlmField(const std::string& fieldName) const noexcept;
    [[nodiscard]] bool hasDuplicatedOcrTokens(const std::string& val) const noexcept;
    [[nodiscard]] bool hasMalformedOcrCharacters(const std::string& val) const noexcept;
    [[nodiscard]] bool hasConflictingEvidence(
        const EvidenceField& field,
        const std::vector<EvidenceField>& allFields
    ) const noexcept;
    [[nodiscard]] bool isStrongTableRowEvidence(
        const EvidenceField& field,
        const EvidenceDocument& doc,
        const std::vector<EvidenceField>& allFields
    ) const noexcept;
};

} // namespace edmo::evidence

#pragma once

#include <ocr/evidence/models/EvidenceDocument.hpp>
#include <ocr/evidence/models/EvidenceField.hpp>
#include <ocr/evidence/patch/LlmPatchTypes.hpp>
#include <ocr/evidence/patch/LlmPatchValidator.hpp>
#include <ocr/evidence/fusion/ConfidenceFusionEngine.hpp>

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

struct EnhancedEdrResult {
    std::string schemaVersion = "edr-enhanced-0.1";
    std::string documentId;
    std::string documentType = "transcript";
    std::vector<EvidenceField> fields;
    PatchAuditReport patchReport;
    nlohmann::json enhancedEdrJson;
    int ocrConfidentCount = 0;
    int deterministicCount = 0;
    int normalizedCount = 0;
    int llmResolvedCount = 0;
    int unresolvedCount = 0;
    int aiUnresolvedCount = 0;
    int rejectedCount = 0;
    bool zeroLossVerified = true;
};

class EdrPatchEngine {
public:
    explicit EdrPatchEngine(ConfidenceFusionEngine fusionEngine = {})
        : fusionEngine_(std::move(fusionEngine)) {}

    /**
     * @brief Applies validated LLM patches to the evidence document, producing Enhanced EDR.
     */
    [[nodiscard]] EnhancedEdrResult applyPatches(
        const EvidenceDocument& doc,
        const std::vector<EvidenceField>& originalFields,
        const LlmPatchDocument& patchDoc
    ) const;

    /**
     * @brief Applies patches directly from JSON.
     */
    [[nodiscard]] EnhancedEdrResult applyPatchesJson(
        const EvidenceDocument& doc,
        const std::vector<EvidenceField>& originalFields,
        const nlohmann::json& patchJson
    ) const;

    [[nodiscard]] const ConfidenceFusionEngine& fusionEngine() const noexcept {
        return fusionEngine_;
    }

    void setFusionEngine(ConfidenceFusionEngine engine) noexcept {
        fusionEngine_ = std::move(engine);
    }

private:
    ConfidenceFusionEngine fusionEngine_;
};

} // namespace ocr::evidence

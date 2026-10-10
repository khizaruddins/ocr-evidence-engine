#pragma once

#include <ocr/evidence/models/EvidenceDocument.hpp>
#include <ocr/evidence/models/EvidenceField.hpp>
#include <ocr/evidence/patch/LlmPatchTypes.hpp>

#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

class LlmPatchValidator {
public:
    LlmPatchValidator() = default;

    /**
     * @brief Validate an untrusted LLM patch document against the canonical document and its fields.
     */
    [[nodiscard]] static PatchAuditReport validatePatches(
        const std::string& canonicalDocumentId,
        const std::vector<EvidenceField>& originalFields,
        const LlmPatchDocument& patchDoc
    );

    /**
     * @brief Validate a raw JSON patch document payload.
     */
    [[nodiscard]] static PatchAuditReport validateJson(
        const std::string& canonicalDocumentId,
        const std::vector<EvidenceField>& originalFields,
        const nlohmann::json& patchJson
    );
};

} // namespace ocr::evidence

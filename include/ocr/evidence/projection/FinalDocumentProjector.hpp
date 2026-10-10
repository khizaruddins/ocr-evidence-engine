#pragma once

#include <edmo/evidence/models/EvidenceDocument.hpp>
#include <edmo/evidence/models/EvidenceField.hpp>
#include <edmo/evidence/patch/EdrPatchEngine.hpp>

#include <chrono>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace edmo::evidence {

struct FinalizationReport {
    std::string schemaVersion = "finalization-report-0.1";
    std::string documentId;
    std::string source = "enhanced_edr";
    bool llmUsed = false;
    int fieldsProjected = 0;
    int fieldsMissing = 0;
    int fieldsWithLlmResolution = 0;
    int fieldsWithDeterministicResolution = 0;
    float overallConfidence = 1.0f;
    int64_t executionTimeMs = 0;
    std::string status = "COMPLETED";

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"schema_version", schemaVersion},
            {"document_id", documentId},
            {"source", source},
            {"llm_used", llmUsed},
            {"fields_projected", fieldsProjected},
            {"fields_missing", fieldsMissing},
            {"fields_with_llm_resolution", fieldsWithLlmResolution},
            {"fields_with_deterministic_resolution", fieldsWithDeterministicResolution},
            {"overall_confidence", overallConfidence},
            {"execution_time_ms", executionTimeMs},
            {"status", status}
        };
    }
};

class FinalDocumentProjector {
public:
    FinalDocumentProjector() = default;

    /**
     * @brief Projects an Enhanced EDR result into a clean semantic final structured document JSON (schema: edmo-final-document-0.1).
     */
    [[nodiscard]] static nlohmann::json project(
        const EnhancedEdrResult& enhancedResult,
        const EvidenceDocument& originalDoc
    );

    /**
     * @brief Projects directly from an enhanced_edr.json payload.
     */
    [[nodiscard]] static nlohmann::json projectFromEnhancedJson(
        const nlohmann::json& enhancedEdrJson
    );

    /**
     * @brief Builds a finalization-report.json audit artifact.
     */
    [[nodiscard]] static FinalizationReport buildFinalizationReport(
        const EnhancedEdrResult& enhancedResult,
        const nlohmann::json& finalDocumentJson,
        int64_t executionTimeMs
    );
};

} // namespace edmo::evidence


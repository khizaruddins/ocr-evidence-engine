#pragma once

#include <ocr/evidence/pipeline/EvidencePipeline.hpp>

#include <filesystem>
#include <string>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

/**
 * @brief Serializes enriched evidence outputs to standard JSON structures and files on disk.
 */
class EvidenceJsonSerializer {
public:
    /**
     * @brief Build evidence-full.json object.
     */
    [[nodiscard]] static nlohmann::json buildFullEvidenceJson(
        const EvidenceDocument& doc,
        const std::vector<EvidenceField>& fields
    );

    /**
     * @brief Build evidence-report.json object.
     */
    [[nodiscard]] static nlohmann::json buildReportJson(
        const PipelineResult& result
    );

    /**
     * @brief Write standard output files (`evidence-full.json`, `evidence-llm.json`, `evidence-report.json`) to directory.
     */
    static bool savePipelineOutputs(
        const PipelineResult& result,
        const std::filesystem::path& outputDir,
        bool prettyPrint = true
    );

    /**
     * @brief Write enhanced output files (`enhanced_edr.json`, `final_document.json`, `patch-report.json`, `finalization-report.json`) to directory.
     */
    static bool saveEnhancedOutputs(
        const nlohmann::json& enhancedEdr,
        const nlohmann::json& finalDocument,
        const nlohmann::json& patchReport,
        const std::filesystem::path& outputDir,
        const nlohmann::json& finalizationReport = {},
        bool prettyPrint = true
    );
};

} // namespace ocr::evidence

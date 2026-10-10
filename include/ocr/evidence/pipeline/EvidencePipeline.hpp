#pragma once

#include <edmo/evidence/confidence/ConfidenceEngine.hpp>
#include <edmo/evidence/confusion/CharacterConfusionMatrix.hpp>
#include <edmo/evidence/context/TargetedLlmContextBuilder.hpp>
#include <edmo/evidence/correction/ContextualCorrectionEngine.hpp>
#include <edmo/evidence/correction/CorrectionEngine.hpp>
#include <edmo/evidence/dictionary/DictionaryEngine.hpp>
#include <edmo/evidence/extraction/TranscriptFieldExtractor.hpp>
#include <edmo/evidence/fusion/EvidenceFusionEngine.hpp>
#include <edmo/evidence/models/EvidenceDocument.hpp>
#include <edmo/evidence/parser/EdrParser.hpp>
#include <edmo/evidence/pipeline/PerformanceTimer.hpp>
#include <edmo/evidence/pipeline/PipelineConfig.hpp>
#include <edmo/evidence/routing/ConfidenceRouter.hpp>
#include <edmo/evidence/structure/FieldCandidateValidator.hpp>
#include <edmo/evidence/validation/ZeroLossAuditor.hpp>

#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace edmo::evidence {

/**
 * @brief Comprehensive pipeline execution result containing all enriched evidence, targeted LLM context, and reports.
 */
struct PipelineResult {
    EvidenceDocument document;
    std::vector<EvidenceField> fields;
    nlohmann::json evidenceFull;
    nlohmann::json evidenceLlm;
    nlohmann::json resolutionRequest;
    nlohmann::json evidenceReport;

    ZeroLossReport zeroLossReport;
    RoutingStats routingStats;
    PerformanceTimer timer;

    int totalFields = 0;
    int highConfidenceFields = 0;
    int mediumConfidenceFields = 0;
    int lowConfidenceFields = 0;
    int correctionsAttempted = 0;
    int correctionsApplied = 0;
    bool llmRequired = false;
    int llmTaskCount = 0;
};

/**
 * @brief Top-level facade coordinating the entire Evidence Intelligence Engine pipeline.
 */
class EvidencePipeline {
public:
    explicit EvidencePipeline(PipelineConfig config = {});

    /**
     * @brief Execute full pipeline on a Full EDR JSON object.
     */
    [[nodiscard]] PipelineResult process(const nlohmann::json& fullEdrJson);

    /**
     * @brief Execute full pipeline on a Full EDR JSON file on disk.
     */
    [[nodiscard]] PipelineResult processFile(const std::filesystem::path& edrFilePath);

    [[nodiscard]] const PipelineConfig& config() const noexcept {
        return config_;
    }

    void setConfig(const PipelineConfig& config);

private:
    PipelineConfig config_;

    std::shared_ptr<DictionaryEngine> dictionary_;
    std::shared_ptr<CharacterConfusionMatrix> confusion_;
    std::shared_ptr<CorrectionEngine> correctionEngine_;
    std::shared_ptr<ContextualCorrectionEngine> contextualCorrectionEngine_;
    std::shared_ptr<ConfidenceEngine> confidenceEngine_;
    std::shared_ptr<EvidenceFusionEngine> fusionEngine_;
    std::shared_ptr<ConfidenceRouter> router_;
    std::shared_ptr<TranscriptFieldExtractor> transcriptExtractor_;
    std::shared_ptr<FieldCandidateValidator> candidateValidator_;

    void initializeComponents();
};

} // namespace edmo::evidence

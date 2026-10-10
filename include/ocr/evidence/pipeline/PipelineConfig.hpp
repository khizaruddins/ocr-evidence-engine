#pragma once

#include <ocr/evidence/confidence/ConfidenceConfig.hpp>
#include <ocr/evidence/routing/RouterConfig.hpp>
#include <ocr/evidence/fusion/ConfidenceFusionEngine.hpp>
#include <ocr/evidence/context/TargetedLlmContextBuilder.hpp>

#include <filesystem>
#include <string>

namespace ocr::evidence {

/**
 * @brief Top-level configuration options for Evidence Intelligence Engine pipeline execution.
 */
struct PipelineConfig {
    std::string documentId;
    std::string documentType = "transcript";
    std::filesystem::path dictionariesDir;
    std::filesystem::path confusionFile;
    std::filesystem::path rulesFile;

    ConfidenceConfig confidenceConfig;
    RouterConfig routerConfig;
    FusionPolicy fusionPolicy;

    ContextLevel contextLevel = ContextLevel::Targeted;

    bool enableCorrection = true;
    bool enableContextualCorrection = true;
    bool enableFusion = true;
    bool enableRouting = true;
    bool verbose = false;
    bool debug = false;
};

} // namespace ocr::evidence

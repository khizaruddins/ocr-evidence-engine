#pragma once

#include <edmo/evidence/confidence/ConfidenceConfig.hpp>
#include <edmo/evidence/routing/RouterConfig.hpp>
#include <edmo/evidence/fusion/ConfidenceFusionEngine.hpp>
#include <edmo/evidence/context/TargetedLlmContextBuilder.hpp>

#include <filesystem>
#include <string>

namespace edmo::evidence {

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

} // namespace edmo::evidence

#pragma once

namespace ocr::evidence {

/**
 * @brief Configurable weights and thresholds for multi-level confidence scoring.
 */
struct ConfidenceConfig {
    float weightOcr = 0.50f;
    float weightLabel = 0.15f;
    float weightStructural = 0.15f;
    float weightContext = 0.20f;

    float lowConfidenceTokenThreshold = 0.80f;
    float lowConfidencePenaltyFactor = 0.25f;

    float highConfidenceThreshold = 0.95f;
    float mediumConfidenceThreshold = 0.90f;
};

} // namespace ocr::evidence

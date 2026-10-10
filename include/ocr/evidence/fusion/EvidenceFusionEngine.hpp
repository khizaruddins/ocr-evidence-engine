#pragma once

#include <edmo/evidence/confidence/ConfidenceEngine.hpp>
#include <edmo/evidence/fusion/MultiOcrHypothesis.hpp>
#include <edmo/evidence/models/EvidenceField.hpp>

#include <memory>

namespace edmo::evidence {

/**
 * @brief Fuses original OCR, correction candidates, dictionary matches, structural cues, and multi-OCR hypotheses.
 */
class EvidenceFusionEngine {
public:
    explicit EvidenceFusionEngine(
        std::shared_ptr<ConfidenceEngine> confidenceEngine = nullptr
    );

    /**
     * @brief Perform multi-signal evidence fusion on a single field.
     */
    [[nodiscard]] FusionResult fuseField(
        const EvidenceField& field,
        float dictionaryScore,
        float fuzzyScore,
        float contextScore,
        float structuralScore,
        const MultiOcrInput& multiOcr = {}
    ) const noexcept;

    /**
     * @brief Synthesize consensus from multiple OCR hypotheses.
     */
    [[nodiscard]] OcrHypothesis fuseMultiOcr(
        const MultiOcrInput& input
    ) const noexcept;

private:
    std::shared_ptr<ConfidenceEngine> confidenceEngine_;
};

} // namespace edmo::evidence

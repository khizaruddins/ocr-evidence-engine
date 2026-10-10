#include <ocr/evidence/fusion/EvidenceFusionEngine.hpp>

#include <algorithm>
#include <cmath>

namespace ocr::evidence {

EvidenceFusionEngine::EvidenceFusionEngine(
    std::shared_ptr<ConfidenceEngine> confidenceEngine
) : confidenceEngine_(confidenceEngine ? confidenceEngine : std::make_shared<ConfidenceEngine>()) {}

FusionResult EvidenceFusionEngine::fuseField(
    const EvidenceField& field,
    float dictionaryScore,
    float fuzzyScore,
    float contextScore,
    float structuralScore,
    const MultiOcrInput& multiOcr
) const noexcept {
    FusionResult result;
    result.originalConfidence = std::clamp(field.original.confidence, 0.0f, 1.0f);
    result.dictionaryScore = std::clamp(dictionaryScore, 0.0f, 1.0f);
    result.fuzzyScore = std::clamp(fuzzyScore, 0.0f, 1.0f);
    result.contextScore = std::clamp(contextScore, 0.0f, 1.0f);
    result.structuralScore = std::clamp(structuralScore, 0.0f, 1.0f);

    // Multi-hypothesis consensus if present
    if (!multiOcr.empty()) {
        auto fusedHyp = fuseMultiOcr(multiOcr);
        result.multiHypothesisScore = fusedHyp.confidence;
    } else {
        result.multiHypothesisScore = 1.0f;
    }

    // Formula weights:
    // w1: OCR confidence (0.45)
    // w2: Dictionary & Fuzzy agreement (0.25)
    // w3: Contextual consistency (0.15)
    // w4: Structural alignment (0.15)
    float baseScore = 0.0f;

    if (field.correction.applied) {
        // If a deterministic correction was applied with high confidence
        baseScore = (0.30f * result.originalConfidence) +
                    (0.35f * field.correction.confidence) +
                    (0.15f * result.contextScore) +
                    (0.20f * result.structuralScore);
    } else {
        baseScore = (0.50f * result.originalConfidence) +
                    (0.20f * result.dictionaryScore) +
                    (0.15f * result.contextScore) +
                    (0.15f * result.structuralScore);
    }

    if (!multiOcr.empty()) {
        baseScore = 0.8f * baseScore + 0.2f * result.multiHypothesisScore;
    }

    if (result.originalConfidence < 0.60f && !field.correction.applied) {
        // Prevent artificial confidence inflation when underlying OCR confidence is low
        baseScore = std::min(baseScore, result.originalConfidence + 0.15f);
        baseScore = std::min(baseScore, 0.75f);
    }

    result.finalConfidence = std::clamp(baseScore, 0.0f, 1.0f);
    return result;
}

OcrHypothesis EvidenceFusionEngine::fuseMultiOcr(
    const MultiOcrInput& input
) const noexcept {
    OcrHypothesis result;
    if (input.empty()) return result;

    // Pick highest confidence hypothesis, or average if matching
    const OcrHypothesis* best = &input.sources[0];
    float totalWeight = 0.0f;
    int matchingCount = 0;

    for (const auto& h : input.sources) {
        if (h.confidence > best->confidence) {
            best = &h;
        }
        if (h.value == input.sources[0].value) {
            matchingCount++;
        }
        totalWeight += h.confidence;
    }

    result.source = "fused_multi_ocr";
    result.value = best->value;

    // Consensus boost if multiple independent passes agree
    if (matchingCount == static_cast<int>(input.sources.size()) && input.sources.size() > 1) {
        result.confidence = std::min(1.0f, best->confidence + 0.05f);
    } else {
        result.confidence = best->confidence;
    }

    return result;
}

} // namespace ocr::evidence

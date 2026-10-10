#include <ocr/evidence/confidence/ConfidenceEngine.hpp>

#include <algorithm>
#include <numeric>

namespace ocr::evidence {

float ConfidenceEngine::calculateFieldConfidence(
    const EvidenceField& field,
    float labelConfidence,
    float structuralConfidence,
    float contextConfidence
) const noexcept {
    float ocrConf = std::clamp(field.original.confidence, 0.0f, 1.0f);
    float labelConf = std::clamp(labelConfidence, 0.0f, 1.0f);
    float structConf = std::clamp(structuralConfidence, 0.0f, 1.0f);
    float ctxConf = std::clamp(contextConfidence, 0.0f, 1.0f);

    float baseScore = (config_.weightOcr * ocrConf) +
                      (config_.weightLabel * labelConf) +
                      (config_.weightStructural * structConf) +
                      (config_.weightContext * ctxConf);

    // Low confidence penalty
    if (field.original.lowConfidenceTokenCount > 0) {
        int tokenCount = static_cast<int>(field.original.rawTokens.size());
        if (tokenCount <= 0) tokenCount = 1;
        float lowRatio = static_cast<float>(field.original.lowConfidenceTokenCount) / static_cast<float>(tokenCount);
        baseScore -= (lowRatio * config_.lowConfidencePenaltyFactor);
    }

    return std::clamp(baseScore, 0.0f, 1.0f);
}

ConfidenceSummary ConfidenceEngine::calculateSummary(
    const std::vector<EvidenceToken>& tokens
) const noexcept {
    ConfidenceSummary summary;
    if (tokens.empty()) return summary;

    summary.tokenCount = static_cast<int>(tokens.size());
    summary.minimum = 1.0f;
    summary.maximum = 0.0f;

    double sum = 0.0;
    int lowCount = 0;

    for (const auto& t : tokens) {
        float c = std::clamp(t.confidence, 0.0f, 1.0f);
        sum += c;
        if (c < summary.minimum) summary.minimum = c;
        if (c > summary.maximum) summary.maximum = c;
        if (c < config_.lowConfidenceTokenThreshold) {
            lowCount++;
        }
    }

    summary.mean = static_cast<float>(sum / tokens.size());
    summary.lowConfidenceCount = lowCount;
    return summary;
}

ConfidenceSummary ConfidenceEngine::calculateTableSummary(
    const EvidenceTable& table
) const noexcept {
    ConfidenceSummary summary;
    int totalCells = 0;
    double sum = 0.0;
    summary.minimum = 1.0f;
    summary.maximum = 0.0f;
    int lowCount = 0;

    for (const auto& row : table.rows) {
        for (const auto& cell : row.cells) {
            float c = std::clamp(cell.confidence, 0.0f, 1.0f);
            sum += c;
            totalCells++;
            if (c < summary.minimum) summary.minimum = c;
            if (c > summary.maximum) summary.maximum = c;
            if (c < config_.lowConfidenceTokenThreshold) {
                lowCount++;
            }
        }
    }

    if (totalCells > 0) {
        summary.tokenCount = totalCells;
        summary.mean = static_cast<float>(sum / totalCells);
        summary.lowConfidenceCount = lowCount;
    }

    return summary;
}

} // namespace ocr::evidence

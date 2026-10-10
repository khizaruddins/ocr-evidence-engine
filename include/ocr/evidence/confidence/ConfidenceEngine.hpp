#pragma once

#include <edmo/evidence/confidence/ConfidenceConfig.hpp>
#include <edmo/evidence/models/EvidenceBlock.hpp>
#include <edmo/evidence/models/EvidenceField.hpp>
#include <edmo/evidence/models/EvidenceTable.hpp>

#include <vector>

namespace edmo::evidence {

/**
 * @brief Multi-level confidence calculation engine for tokens, blocks, cells, rows, fields, and documents.
 */
class ConfidenceEngine {
public:
    explicit ConfidenceEngine(ConfidenceConfig config = {})
        : config_(config) {}

    /**
     * @brief Calculate multi-factor field confidence score.
     */
    [[nodiscard]] float calculateFieldConfidence(
        const EvidenceField& field,
        float labelConfidence = 1.0f,
        float structuralConfidence = 1.0f,
        float contextConfidence = 1.0f
    ) const noexcept;

    /**
     * @brief Aggregate token confidence statistics into a ConfidenceSummary.
     */
    [[nodiscard]] ConfidenceSummary calculateSummary(
        const std::vector<EvidenceToken>& tokens
    ) const noexcept;

    /**
     * @brief Aggregate cell confidence statistics across an entire table.
     */
    [[nodiscard]] ConfidenceSummary calculateTableSummary(
        const EvidenceTable& table
    ) const noexcept;

    [[nodiscard]] const ConfidenceConfig& config() const noexcept {
        return config_;
    }

    void setConfig(const ConfidenceConfig& config) noexcept {
        config_ = config;
    }

private:
    ConfidenceConfig config_;
};

} // namespace edmo::evidence

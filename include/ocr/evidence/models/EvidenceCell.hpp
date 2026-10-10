#pragma once

#include <edmo/evidence/models/EvidenceSource.hpp>
#include <edmo/evidence/models/EvidenceToken.hpp>
#include <edmo/evidence/models/Fates.hpp>

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace edmo::evidence {

/**
 * @brief Represents an individual cell within an evidence table.
 */
struct EvidenceCell {
    std::string text;
    float confidence = 1.0f;
    int colSpan = 1;
    int rowSpan = 1;
    int colIndex = 0;
    int rowIndex = 0;
    std::string columnName;
    std::string canonicalColumnName;

    EvidenceSource source;
    std::vector<EvidenceToken> tokens;
    EvidenceFate fate = EvidenceFate::Structured;

    [[nodiscard]] bool empty() const noexcept {
        return text.empty();
    }

    [[nodiscard]] nlohmann::json toJson(bool includeSource = true) const {
        nlohmann::json j{
            {"text", text},
            {"confidence", confidence},
            {"fate", fateToString(fate)}
        };
        if (colSpan > 1) j["colspan"] = colSpan;
        if (rowSpan > 1) j["rowspan"] = rowSpan;
        if (!canonicalColumnName.empty()) {
            j["canonical_name"] = canonicalColumnName;
        }
        if (includeSource && !source.empty()) {
            j["source"] = source.toJson();
        }
        return j;
    }
};

} // namespace edmo::evidence

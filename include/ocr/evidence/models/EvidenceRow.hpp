#pragma once

#include <edmo/evidence/models/EvidenceCell.hpp>
#include <edmo/evidence/models/EvidenceSource.hpp>
#include <edmo/evidence/models/Fates.hpp>

#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace edmo::evidence {

/**
 * @brief Represents a row in an evidence table with cell collections and canonical field mappings.
 */
struct EvidenceRow {
    int rowIndex = 0;
    bool isHeader = false;
    float confidence = 1.0f;
    std::vector<EvidenceCell> cells;
    std::map<std::string, std::string> canonicalFields;
    EvidenceSource source;
    EvidenceFate fate = EvidenceFate::Structured;

    [[nodiscard]] bool empty() const noexcept {
        return cells.empty() && canonicalFields.empty();
    }

    [[nodiscard]] nlohmann::json toJson(bool includeSource = true) const {
        nlohmann::json j;
        if (!canonicalFields.empty()) {
            for (const auto& [k, v] : canonicalFields) {
                j[k] = v;
            }
        }
        if (!cells.empty()) {
            nlohmann::json cellsJson = nlohmann::json::array();
            for (const auto& cell : cells) {
                cellsJson.push_back(cell.toJson(includeSource));
            }
            j["cells"] = std::move(cellsJson);
        }
        if (isHeader) {
            j["is_header"] = true;
        }
        j["confidence"] = confidence;
        j["fate"] = fateToString(fate);
        if (includeSource && !source.empty()) {
            j["source"] = source.toJson();
        }
        return j;
    }
};

} // namespace edmo::evidence

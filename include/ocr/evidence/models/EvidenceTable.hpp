#pragma once

#include <ocr/evidence/models/EvidenceRow.hpp>
#include <ocr/evidence/models/EvidenceSource.hpp>
#include <ocr/evidence/models/EvidenceToken.hpp>
#include <ocr/evidence/models/Fates.hpp>

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

struct EvidenceColumn {
    std::string name;
    std::string canonicalName;
    int columnIndex = 0;
    float confidence = 1.0f;
    EvidenceSource source;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"name", name},
            {"canonical_name", canonicalName.empty() ? name : canonicalName},
            {"column_index", columnIndex},
            {"confidence", confidence}
        };
    }
};

/**
 * @brief Represents a reconstructed semantic table spanning pages with rows and columns.
 */
struct EvidenceTable {
    std::string id;
    std::string tableType = "generic_table";
    std::string caption;
    int pageStart = 1;
    int pageEnd = 1;
    bool hasHeader = false;

    std::vector<EvidenceColumn> columns;
    std::vector<EvidenceRow> rows;
    ConfidenceSummary confidence;
    EvidenceSource source;
    EvidenceFate fate = EvidenceFate::Structured;

    [[nodiscard]] bool empty() const noexcept {
        return rows.empty() && columns.empty();
    }

    [[nodiscard]] nlohmann::json toJson(bool includeSource = true) const {
        nlohmann::json j{
            {"id", id},
            {"table_type", tableType},
            {"page_start", pageStart},
            {"page_end", pageEnd},
            {"has_header", hasHeader},
            {"confidence", confidence.toJson()},
            {"fate", fateToString(fate)}
        };

        if (!caption.empty()) {
            j["caption"] = caption;
        }

        nlohmann::json colsJson = nlohmann::json::array();
        for (const auto& col : columns) {
            colsJson.push_back(col.toJson());
        }
        j["columns"] = std::move(colsJson);

        nlohmann::json rowsJson = nlohmann::json::array();
        for (const auto& row : rows) {
            rowsJson.push_back(row.toJson(includeSource));
        }
        j["rows"] = std::move(rowsJson);

        if (includeSource && !source.empty()) {
            j["source"] = source.toJson();
        }

        return j;
    }
};

} // namespace ocr::evidence

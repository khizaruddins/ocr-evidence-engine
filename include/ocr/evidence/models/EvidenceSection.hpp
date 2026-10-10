#pragma once

#include <ocr/evidence/models/EvidenceBlock.hpp>
#include <ocr/evidence/models/EvidenceSource.hpp>
#include <ocr/evidence/models/EvidenceToken.hpp>
#include <ocr/evidence/models/Fates.hpp>

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

/**
 * @brief Represents a semantic section of a document.
 */
struct EvidenceSection {
    std::string id;
    std::string heading;
    std::string sectionType = "body";
    int pageStart = 1;
    int pageEnd = 1;

    std::vector<EvidenceBlock> blocks;
    ConfidenceSummary confidence;
    EvidenceSource source;
    EvidenceFate fate = EvidenceFate::Structured;

    [[nodiscard]] nlohmann::json toJson(bool includeSource = true) const {
        nlohmann::json j{
            {"id", id},
            {"heading", heading},
            {"section_type", sectionType},
            {"page_start", pageStart},
            {"page_end", pageEnd},
            {"confidence", confidence.toJson()},
            {"fate", fateToString(fate)}
        };

        nlohmann::json blocksJson = nlohmann::json::array();
        for (const auto& b : blocks) {
            blocksJson.push_back(b.toJson(includeSource));
        }
        j["blocks"] = std::move(blocksJson);

        if (includeSource && !source.empty()) {
            j["source"] = source.toJson();
        }

        return j;
    }
};

} // namespace ocr::evidence

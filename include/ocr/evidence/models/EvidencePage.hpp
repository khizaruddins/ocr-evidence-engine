#pragma once

#include <ocr/evidence/models/EvidenceBlock.hpp>
#include <ocr/evidence/models/EvidenceToken.hpp>
#include <ocr/evidence/models/Fates.hpp>

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

/**
 * @brief Represents a physical page with its blocks and layout geometry.
 */
struct EvidencePage {
    int pageNumber = 1;
    int width = 0;
    int height = 0;
    int rotation = 0;

    std::vector<EvidenceBlock> blocks;
    ConfidenceSummary confidence;
    EvidenceFate fate = EvidenceFate::Preserved;

    [[nodiscard]] nlohmann::json toJson(bool includeSource = true) const {
        nlohmann::json j{
            {"page_number", pageNumber},
            {"width", width},
            {"height", height},
            {"rotation", rotation},
            {"confidence", confidence.toJson()},
            {"fate", fateToString(fate)}
        };

        nlohmann::json blocksJson = nlohmann::json::array();
        for (const auto& b : blocks) {
            blocksJson.push_back(b.toJson(includeSource));
        }
        j["blocks"] = std::move(blocksJson);

        return j;
    }
};

} // namespace ocr::evidence

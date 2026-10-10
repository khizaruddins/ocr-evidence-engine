#pragma once

#include <ocr/evidence/models/EvidenceSource.hpp>
#include <ocr/evidence/models/EvidenceToken.hpp>
#include <ocr/evidence/models/Fates.hpp>

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

/**
 * @brief Block type classifying structural role in document layout.
 */
enum class BlockType {
    Header,
    Footer,
    Heading,
    Paragraph,
    LabelValue,
    List,
    TableCell,
    Noise,
    Unknown
};

[[nodiscard]] inline std::string blockTypeToString(BlockType t) noexcept {
    switch (t) {
        case BlockType::Header:     return "header";
        case BlockType::Footer:     return "footer";
        case BlockType::Heading:    return "heading";
        case BlockType::Paragraph:  return "paragraph";
        case BlockType::LabelValue: return "label_value";
        case BlockType::List:       return "list";
        case BlockType::TableCell:  return "table_cell";
        case BlockType::Noise:      return "noise";
        case BlockType::Unknown:    return "unknown";
    }
    return "unknown";
}

[[nodiscard]] inline BlockType blockTypeFromString(std::string_view sv) noexcept {
    if (sv == "header")      return BlockType::Header;
    if (sv == "footer")      return BlockType::Footer;
    if (sv == "heading")     return BlockType::Heading;
    if (sv == "paragraph")   return BlockType::Paragraph;
    if (sv == "label_value") return BlockType::LabelValue;
    if (sv == "list")        return BlockType::List;
    if (sv == "table_cell")  return BlockType::TableCell;
    if (sv == "noise")       return BlockType::Noise;
    return BlockType::Unknown;
}

/**
 * @brief Represents a semantic block of text parsed from Full EDR.
 */
struct EvidenceBlock {
    std::string id;
    std::string text;
    BlockType type = BlockType::Unknown;

    std::string label;
    std::string value;

    ConfidenceSummary confidence;
    EvidenceSource source;
    std::vector<EvidenceToken> tokens;
    EvidenceFate fate = EvidenceFate::Preserved;

    [[nodiscard]] bool empty() const noexcept {
        return text.empty() && label.empty() && value.empty();
    }

    [[nodiscard]] nlohmann::json toJson(bool includeSource = true) const {
        nlohmann::json j{
            {"id", id},
            {"type", blockTypeToString(type)},
            {"text", text},
            {"confidence", confidence.toJson()},
            {"fate", fateToString(fate)}
        };

        if (type == BlockType::LabelValue || (!label.empty() && !value.empty())) {
            j["label"] = label;
            j["value"] = value;
        }

        if (includeSource && !source.empty()) {
            j["source"] = source.toJson();
        }

        return j;
    }
};

} // namespace ocr::evidence

#pragma once

#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace edmo::evidence {

/**
 * @brief 2D bounding box representing spatial layout geometry.
 */
struct BoundingBox {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    [[nodiscard]] bool empty() const noexcept {
        return width <= 0 || height <= 0;
    }

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"x", x},
            {"y", y},
            {"width", width},
            {"height", height}
        };
    }

    static BoundingBox fromJson(const nlohmann::json& j) {
        BoundingBox box;
        if (j.is_object()) {
            box.x = j.value("x", 0);
            box.y = j.value("y", 0);
            box.width = j.value("width", 0);
            box.height = j.value("height", 0);
        }
        return box;
    }
};

/**
 * @brief Complete provenance linking an evidence item back to physical pages, boxes, OCR IDs, and table structures.
 */
struct EvidenceSource {
    int page = 1;
    BoundingBox bbox;
    std::vector<std::string> ocrIds;
    std::string tableId;
    int row = -1;
    int cell = -1;
    std::string field;

    [[nodiscard]] bool empty() const noexcept {
        return page <= 0 && bbox.empty() && ocrIds.empty() && tableId.empty();
    }

    [[nodiscard]] nlohmann::json toJson(bool includeOcrIds = true) const {
        nlohmann::json j{
            {"page", page}
        };
        if (!bbox.empty()) {
            j["bbox"] = bbox.toJson();
        }
        if (includeOcrIds && !ocrIds.empty()) {
            j["ocr_ids"] = ocrIds;
        }
        if (!tableId.empty()) {
            j["table"] = tableId;
        }
        if (row >= 0) {
            j["row"] = row;
        }
        if (cell >= 0) {
            j["cell"] = cell;
        }
        if (!field.empty()) {
            j["field"] = field;
        }
        return j;
    }

    static EvidenceSource fromJson(const nlohmann::json& j) {
        EvidenceSource src;
        if (!j.is_object()) return src;

        src.page = j.value("page", 1);
        if (j.contains("bbox")) {
            src.bbox = BoundingBox::fromJson(j["bbox"]);
        }
        if (j.contains("ocr_ids") && j["ocr_ids"].is_array()) {
            for (const auto& item : j["ocr_ids"]) {
                if (item.is_string()) src.ocrIds.push_back(item.get<std::string>());
            }
        }
        src.tableId = j.value("table", "");
        src.row = j.value("row", -1);
        src.cell = j.value("cell", -1);
        src.field = j.value("field", "");
        return src;
    }
};

} // namespace edmo::evidence

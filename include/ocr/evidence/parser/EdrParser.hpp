#pragma once

#include <edmo/evidence/models/EvidenceDocument.hpp>

#include <filesystem>
#include <string>
#include <nlohmann/json.hpp>

namespace edmo::evidence {

/**
 * @brief Zero-loss parser for Full EDR JSON documents produced by EDMO OCR Engine.
 */
class EdrParser {
public:
    /**
     * @brief Parse Full EDR JSON object into an EvidenceDocument model.
     */
    [[nodiscard]] static EvidenceDocument parseJson(const nlohmann::json& root);

    /**
     * @brief Parse Full EDR JSON file from disk into an EvidenceDocument model.
     */
    [[nodiscard]] static EvidenceDocument parseFile(const std::filesystem::path& path);
};

} // namespace edmo::evidence

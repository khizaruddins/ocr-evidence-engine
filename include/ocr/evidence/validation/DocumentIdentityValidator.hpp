#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

struct DocumentIdentityValidationResult {
    bool valid = true;
    std::string canonicalDocumentId;
    std::string errorCode;
    std::string errorMessage;
    std::vector<std::pair<std::string, std::string>> observedIds; // source -> id

    [[nodiscard]] nlohmann::json toJson() const {
        if (valid) {
            return nlohmann::json{
                {"status", "VALID"},
                {"document_id", canonicalDocumentId}
            };
        }
        nlohmann::json obs = nlohmann::json::object();
        for (const auto& [src, id] : observedIds) {
            obs[src] = id;
        }
        return nlohmann::json{
            {"error", {
                {"code", errorCode.empty() ? "DOCUMENT_ID_MISMATCH" : errorCode},
                {"message", errorMessage.empty() ? "Document identity mismatch detected across pipeline artifacts." : errorMessage},
                {"canonical_id", canonicalDocumentId},
                {"observed_ids", obs}
            }}
        };
    }
};

class DocumentIdentityValidator {
public:
    DocumentIdentityValidator() = default;

    /**
     * @brief Validate that an artifact's document_id matches the canonical document_id.
     */
    static DocumentIdentityValidationResult validateSingle(
        const std::string& canonicalId,
        const std::string& artifactId,
        const std::string& artifactSource = "artifact"
    );

    /**
     * @brief Validate that all artifact document_ids across the pipeline match.
     */
    static DocumentIdentityValidationResult validateAcrossArtifacts(
        const std::string& canonicalId,
        const std::vector<std::pair<std::string, std::string>>& artifactIds
    );

    /**
     * @brief Extract document_id from JSON object (checks document_id, document.id, etc.)
     */
    static std::string extractDocumentId(const nlohmann::json& j);
};

} // namespace ocr::evidence

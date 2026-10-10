#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

struct FinalValidationResult {
    bool valid = true;
    std::string documentId;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"valid", valid},
            {"document_id", documentId},
            {"errors", errors},
            {"warnings", warnings}
        };
    }
};

class FinalDocumentValidator {
public:
    FinalDocumentValidator() = default;

    /**
     * @brief Validates a Final Document JSON against specification and source Enhanced EDR.
     */
    [[nodiscard]] static FinalValidationResult validate(
        const nlohmann::json& finalDoc,
        const nlohmann::json& enhancedEdr,
        const std::string& expectedDocumentId = ""
    );
};

} // namespace ocr::evidence

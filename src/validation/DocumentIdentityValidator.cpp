#include <ocr/evidence/validation/DocumentIdentityValidator.hpp>

namespace ocr::evidence {

DocumentIdentityValidationResult DocumentIdentityValidator::validateSingle(
    const std::string& canonicalId,
    const std::string& artifactId,
    const std::string& artifactSource
) {
    DocumentIdentityValidationResult result;
    result.canonicalDocumentId = canonicalId;
    result.observedIds.push_back({artifactSource, artifactId});

    if (canonicalId.empty()) {
        result.valid = false;
        result.errorCode = "DOCUMENT_ID_EMPTY";
        result.errorMessage = "Canonical document ID is empty.";
        return result;
    }

    if (artifactId.empty()) {
        result.valid = false;
        result.errorCode = "DOCUMENT_ID_MISSING";
        result.errorMessage = "Document ID is missing in artifact: " + artifactSource;
        return result;
    }

    if (canonicalId != artifactId) {
        result.valid = false;
        result.errorCode = "DOCUMENT_ID_MISMATCH";
        result.errorMessage = "Document identity mismatch detected across pipeline artifacts: expected '" +
                              canonicalId + "', found '" + artifactId + "' in " + artifactSource;
        return result;
    }

    result.valid = true;
    return result;
}

DocumentIdentityValidationResult DocumentIdentityValidator::validateAcrossArtifacts(
    const std::string& canonicalId,
    const std::vector<std::pair<std::string, std::string>>& artifactIds
) {
    DocumentIdentityValidationResult result;
    result.canonicalDocumentId = canonicalId;
    result.observedIds = artifactIds;

    if (canonicalId.empty()) {
        result.valid = false;
        result.errorCode = "DOCUMENT_ID_EMPTY";
        result.errorMessage = "Canonical document ID is empty.";
        return result;
    }

    for (const auto& [src, id] : artifactIds) {
        if (id != canonicalId) {
            result.valid = false;
            result.errorCode = "DOCUMENT_ID_MISMATCH";
            result.errorMessage = "Document identity mismatch detected across pipeline artifacts: expected '" +
                                  canonicalId + "', found '" + id + "' in " + src;
            return result;
        }
    }

    result.valid = true;
    return result;
}

std::string DocumentIdentityValidator::extractDocumentId(const nlohmann::json& j) {
    if (!j.is_object()) return "";

    if (j.contains("document_id") && j["document_id"].is_string()) {
        return j["document_id"].get<std::string>();
    }
    if (j.contains("document") && j["document"].is_object()) {
        if (j["document"].contains("id") && j["document"]["id"].is_string()) {
            return j["document"]["id"].get<std::string>();
        }
        if (j["document"].contains("document_id") && j["document"]["document_id"].is_string()) {
            return j["document"]["document_id"].get<std::string>();
        }
    }
    if (j.contains("metadata") && j["metadata"].is_object()) {
        if (j["metadata"].contains("id") && j["metadata"]["id"].is_string()) {
            return j["metadata"]["id"].get<std::string>();
        }
        if (j["metadata"].contains("document_id") && j["metadata"]["document_id"].is_string()) {
            return j["metadata"]["document_id"].get<std::string>();
        }
    }
    return "";
}

} // namespace ocr::evidence

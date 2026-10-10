#include <ocr/evidence/validation/FinalDocumentValidator.hpp>

namespace ocr::evidence {

FinalValidationResult FinalDocumentValidator::validate(
    const nlohmann::json& finalDoc,
    const nlohmann::json& enhancedEdr,
    const std::string& expectedDocumentId
) {
    FinalValidationResult res;

    // 1. Check Schema Version
    std::string schemaVer = finalDoc.value("schema_version", "");
    if (schemaVer != "ocr-final-document-0.1" && schemaVer != "edmo-final-document-0.1") {
        res.errors.push_back("Invalid schema_version: expected 'edmo-final-document-0.1', got '" + schemaVer + "'");
    }

    // 2. Check Document ID & Document Metadata
    std::string docId = "";
    if (finalDoc.contains("document") && finalDoc["document"].is_object()) {
        docId = finalDoc["document"].value("document_id", "");
    } else if (finalDoc.contains("document_id") && finalDoc["document_id"].is_string()) {
        docId = finalDoc["document_id"].get<std::string>();
    }

    res.documentId = docId;

    if (docId.empty()) {
        res.errors.push_back("Missing document_id in final document");
    }

    if (!expectedDocumentId.empty() && docId != expectedDocumentId) {
        res.errors.push_back("DOCUMENT_ID_MISMATCH: final document_id '" + docId +
                             "' does not match expected canonical ID '" + expectedDocumentId + "'");
    }

    std::string enhancedDocId = enhancedEdr.value("document_id", "");
    if (!enhancedDocId.empty() && docId != enhancedDocId) {
        res.errors.push_back("DOCUMENT_ID_MISMATCH: final document_id '" + docId +
                             "' does not match Enhanced EDR document_id '" + enhancedDocId + "'");
    }

    // 3. Check Confidence Range & Method
    if (finalDoc.contains("confidence") && finalDoc["confidence"].is_object()) {
        const auto& confObj = finalDoc["confidence"];
        if (confObj.contains("overall") && confObj["overall"].is_number()) {
            float overall = confObj["overall"].get<float>();
            if (overall < 0.0f || overall > 1.0f) {
                res.errors.push_back("Confidence value out of range [0.0, 1.0]: overall=" + std::to_string(overall));
            }
        }
        if (confObj.contains("average") && confObj["average"].is_number()) {
            float avg = confObj["average"].get<float>();
            if (avg < 0.0f || avg > 1.0f) {
                res.errors.push_back("Confidence value out of range [0.0, 1.0]: average=" + std::to_string(avg));
            }
        }
        if (confObj.contains("minimum") && confObj["minimum"].is_number()) {
            float minConf = confObj["minimum"].get<float>();
            if (minConf < 0.0f || minConf > 1.0f) {
                res.errors.push_back("Confidence value out of range [0.0, 1.0]: minimum=" + std::to_string(minConf));
            }
        }
    }

    // 4. Validate Course Structures (if transcript)
    if (finalDoc.contains("courses") && finalDoc["courses"].is_array()) {
        for (size_t i = 0; i < finalDoc["courses"].size(); ++i) {
            const auto& c = finalDoc["courses"][i];
            if (!c.is_object()) {
                res.errors.push_back("Course entry " + std::to_string(i) + " is not a valid JSON object");
                continue;
            }

            if (c.contains("confidence") && c["confidence"].is_number()) {
                float conf = c["confidence"].get<float>();
                if (conf < 0.0f || conf > 1.0f) {
                    res.errors.push_back("Course " + std::to_string(i) + " confidence out of range: " + std::to_string(conf));
                }
            }

            if (c.contains("credits") && !c["credits"].is_null() && !c["credits"].is_number()) {
                res.errors.push_back("Course " + std::to_string(i) + " credits must be numeric or null");
            }
        }
    }

    res.valid = res.errors.empty();
    return res;
}

} // namespace ocr::evidence

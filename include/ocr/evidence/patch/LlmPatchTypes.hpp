#pragma once

#include <ocr/evidence/models/EvidenceSource.hpp>

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

/**
 * @brief Source provenance coordinates in an LLM patch.
 */
struct LlmPatchSource {
    int page = 1;
    BoundingBox bbox;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"page", page},
            {"bbox", bbox.toJson()}
        };
    }

    [[nodiscard]] static LlmPatchSource fromJson(const nlohmann::json& j) {
        LlmPatchSource src;
        if (!j.is_object()) return src;
        src.page = j.value("page", 1);
        if (j.contains("bbox") && j["bbox"].is_object()) {
            src.bbox = BoundingBox::fromJson(j["bbox"]);
        }
        return src;
    }
};

/**
 * @brief Single targeted field correction patch returned by LLM.
 */
struct LlmPatch {
    std::string patchId;
    std::string taskId;
    LlmPatchSource source;
    std::string field;
    std::string originalValue;
    std::string correctedValue;
    float llmConfidence = 0.0f;
    std::string reasonCode;
    bool evidenceSupported = true;
    nlohmann::json rawJson = nlohmann::json::object();

    std::string resolutionStatus = "RESOLVED";
    std::string resolutionMethod = "LLM";

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"patch_id", patchId},
            {"task_id", taskId},
            {"status", resolutionStatus},
            {"resolution_method", resolutionMethod},
            {"source", source.toJson()},
            {"field", field},
            {"original_value", originalValue},
            {"corrected_value", correctedValue},
            {"value", correctedValue},
            {"confidence", llmConfidence},
            {"llm_confidence", llmConfidence},
            {"reason_code", reasonCode},
            {"reason", reasonCode},
            {"evidence_supported", evidenceSupported}
        };
    }

    [[nodiscard]] static LlmPatch fromJson(const nlohmann::json& j) {
        LlmPatch p;
        if (!j.is_object()) return p;
        p.rawJson = j;
        p.patchId = j.value("patch_id", j.value("task_id", ""));
        p.taskId = j.value("task_id", j.value("patch_id", ""));
        p.field = j.value("field", "");
        p.originalValue = j.value("original_value", "");
        p.correctedValue = j.value("corrected_value", j.value("value", ""));
        p.llmConfidence = j.value("confidence", j.value("llm_confidence", 0.0f));
        p.reasonCode = j.value("reason_code", j.value("reason", "OCR_CORRECTION"));
        p.resolutionStatus = j.value("status", "RESOLVED");
        p.resolutionMethod = j.value("resolution_method", "LLM");
        p.evidenceSupported = j.value("evidence_supported", true);
        if (j.contains("source") && j["source"].is_object()) {
            p.source = LlmPatchSource::fromJson(j["source"]);
        }
        return p;
    }
};

/**
 * @brief LLM provider and model identifier.
 */
struct LlmModelInfo {
    std::string provider = "unknown";
    std::string model = "unknown";

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"provider", provider},
            {"model", model}
        };
    }

    [[nodiscard]] static LlmModelInfo fromJson(const nlohmann::json& j) {
        LlmModelInfo m;
        if (!j.is_object()) return m;
        m.provider = j.value("provider", "unknown");
        m.model = j.value("model", "unknown");
        return m;
    }
};

/**
 * @brief Root payload representing an LLM Patch / Resolution Response document.
 */
struct LlmPatchDocument {
    std::string schemaVersion = "ocr-llm-resolution-response-0.1";
    std::string documentId;
    LlmModelInfo model;
    std::vector<LlmPatch> patches;

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json patchArray = nlohmann::json::array();
        for (const auto& p : patches) {
            patchArray.push_back(p.toJson());
        }
        return nlohmann::json{
            {"schema_version", schemaVersion},
            {"document_id", documentId},
            {"model", model.toJson()},
            {"resolutions", patchArray},
            {"patches", std::move(patchArray)}
        };
    }

    [[nodiscard]] static LlmPatchDocument fromJson(const nlohmann::json& j) {
        LlmPatchDocument doc;
        if (!j.is_object()) return doc;
        doc.schemaVersion = j.value("schema_version", "ocr-llm-resolution-response-0.1");
        doc.documentId = j.value("document_id", "");
        if (j.contains("model") && j["model"].is_object()) {
            doc.model = LlmModelInfo::fromJson(j["model"]);
        }
        if (j.contains("resolutions") && j["resolutions"].is_array()) {
            for (const auto& rj : j["resolutions"]) {
                doc.patches.push_back(LlmPatch::fromJson(rj));
            }
        } else if (j.contains("patches") && j["patches"].is_array()) {
            for (const auto& pj : j["patches"]) {
                doc.patches.push_back(LlmPatch::fromJson(pj));
            }
        }
        return doc;
    }
};

/**
 * @brief Validation status for a single patch.
 */
enum class PatchValidationStatus {
    Accepted,
    Rejected
};

struct PatchValidationItem {
    std::string patchId;
    std::string taskId;
    std::string field;
    PatchValidationStatus status = PatchValidationStatus::Rejected;
    std::string reasonCode;
    std::string reasonMessage;
    std::string originalValue;
    std::string correctedValue;
    float finalConfidence = 0.0f;

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json j{
            {"patch_id", patchId},
            {"task_id", taskId},
            {"field", field},
            {"status", status == PatchValidationStatus::Accepted ? "ACCEPTED" : "REJECTED"}
        };
        if (!reasonCode.empty()) j["reason_code"] = reasonCode;
        if (!reasonMessage.empty()) j["reason"] = reasonMessage;
        if (!originalValue.empty()) j["original_value"] = originalValue;
        if (!correctedValue.empty()) j["corrected_value"] = correctedValue;
        if (status == PatchValidationStatus::Accepted) j["final_confidence"] = finalConfidence;
        return j;
    }
};

struct PatchAuditReport {
    std::string documentId;
    int received = 0;
    int accepted = 0;
    int rejected = 0;
    std::vector<PatchValidationItem> acceptedPatches;
    std::vector<PatchValidationItem> rejectedPatches;

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json acc = nlohmann::json::array();
        for (const auto& p : acceptedPatches) {
            acc.push_back({
                {"patch_id", p.patchId},
                {"task_id", p.taskId},
                {"field", p.field},
                {"original_value", p.originalValue},
                {"corrected_value", p.correctedValue},
                {"final_confidence", p.finalConfidence}
            });
        }

        nlohmann::json rej = nlohmann::json::array();
        for (const auto& p : rejectedPatches) {
            rej.push_back({
                {"patch_id", p.patchId},
                {"task_id", p.taskId},
                {"field", p.field},
                {"reason_code", p.reasonCode},
                {"reason", p.reasonMessage}
            });
        }

        return nlohmann::json{
            {"document_id", documentId},
            {"patches", {
                {"received", received},
                {"accepted", accepted},
                {"rejected", rejected}
            }},
            {"accepted", std::move(acc)},
            {"rejected", std::move(rej)}
        };
    }
};

} // namespace ocr::evidence

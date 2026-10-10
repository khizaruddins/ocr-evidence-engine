#include <ocr/evidence/patch/LlmPatchValidator.hpp>
#include <ocr/evidence/normalization/FieldValueNormalizer.hpp>

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace ocr::evidence {

namespace {

std::string trim(std::string_view sv) {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }
    return std::string(sv);
}

} // namespace

PatchAuditReport LlmPatchValidator::validateJson(
    const std::string& canonicalDocumentId,
    const std::vector<EvidenceField>& originalFields,
    const nlohmann::json& patchJson
) {
    if (!patchJson.is_object()) {
        PatchAuditReport rep;
        rep.documentId = canonicalDocumentId;
        rep.received = 0;
        rep.accepted = 0;
        rep.rejected = 1;

        PatchValidationItem item;
        item.patchId = "unknown";
        item.status = PatchValidationStatus::Rejected;
        item.reasonCode = "INVALID_SCHEMA";
        item.reasonMessage = "Patch JSON root payload is not a valid JSON object.";
        rep.rejectedPatches.push_back(std::move(item));
        return rep;
    }

    auto doc = LlmPatchDocument::fromJson(patchJson);
    return validatePatches(canonicalDocumentId, originalFields, doc);
}

PatchAuditReport LlmPatchValidator::validatePatches(
    const std::string& canonicalDocumentId,
    const std::vector<EvidenceField>& originalFields,
    const LlmPatchDocument& patchDoc
) {
    PatchAuditReport report;
    report.documentId = canonicalDocumentId;
    report.received = static_cast<int>(patchDoc.patches.size());

    // 1. Build index of authorized tasks from original fields
    std::map<std::string, const EvidenceField*> taskIndex;
    for (const auto& f : originalFields) {
        if (!f.llmTaskId.empty()) {
            taskIndex[f.llmTaskId] = &f;
        }
        // Also index by fieldId in case tasks were named after fieldId
        taskIndex[f.fieldId] = &f;
    }

    // 2. Global Document ID Check
    bool documentIdMismatch = (!canonicalDocumentId.empty() &&
                               !patchDoc.documentId.empty() &&
                               patchDoc.documentId != canonicalDocumentId);

    // 3. Global Schema Check
    bool validSchemaVersion = (patchDoc.schemaVersion == "ocr-llm-patch-0.1" ||
                               patchDoc.schemaVersion == "ocr-llm-resolution-response-0.1" ||
                               patchDoc.schemaVersion == "edmo-llm-patch-0.1" ||
                               patchDoc.schemaVersion == "edmo-llm-resolution-response-0.1" ||
                               patchDoc.schemaVersion.find("ocr-llm-") != std::string::npos ||
                               patchDoc.schemaVersion.find("edmo-llm-") != std::string::npos);

    std::set<std::string> seenPatchIds;
    std::map<std::string, std::pair<std::string, size_t>> taskAcceptedMap; // taskId -> {correctedValue, acceptedReportIndex}

    for (const auto& patch : patchDoc.patches) {
        PatchValidationItem item;
        item.patchId = patch.patchId;
        item.taskId = patch.taskId;
        item.field = patch.field;
        item.originalValue = patch.originalValue;
        item.correctedValue = patch.correctedValue;
        item.status = PatchValidationStatus::Rejected;

        // Check 1: Document ID Mismatch
        if (documentIdMismatch || (patchDoc.documentId.empty() && !canonicalDocumentId.empty())) {
            item.reasonCode = "DOCUMENT_ID_MISMATCH";
            item.reasonMessage = "Patch document ID '" + patchDoc.documentId +
                                 "' does not match canonical document ID '" + canonicalDocumentId + "'.";
            report.rejectedPatches.push_back(std::move(item));
            continue;
        }

        // Check 2: Invalid Schema Version
        if (!validSchemaVersion) {
            item.reasonCode = "INVALID_SCHEMA";
            item.reasonMessage = "Unsupported patch schema version: " + patchDoc.schemaVersion;
            report.rejectedPatches.push_back(std::move(item));
            continue;
        }

        // Check 3: Empty Patch ID
        if (patch.patchId.empty()) {
            item.reasonCode = "INVALID_SCHEMA";
            item.reasonMessage = "Patch ID must not be empty.";
            report.rejectedPatches.push_back(std::move(item));
            continue;
        }

        // Check 4: Duplicate Patch ID
        if (seenPatchIds.count(patch.patchId) > 0) {
            item.reasonCode = "DUPLICATE_PATCH";
            item.reasonMessage = "Duplicate patch ID encountered: " + patch.patchId;
            report.rejectedPatches.push_back(std::move(item));
            continue;
        }
        seenPatchIds.insert(patch.patchId);

        // Check 5: Task ID Existence
        if (patch.taskId.empty()) {
            item.reasonCode = "UNKNOWN_TASK";
            item.reasonMessage = "Patch task_id is empty.";
            report.rejectedPatches.push_back(std::move(item));
            continue;
        }

        auto it = taskIndex.find(patch.taskId);
        if (it == taskIndex.end()) {
            item.reasonCode = "UNKNOWN_TASK";
            item.reasonMessage = "Task ID '" + patch.taskId + "' does not correspond to any authorized evidence task.";
            report.rejectedPatches.push_back(std::move(item));
            continue;
        }

        const EvidenceField* targetField = it->second;

        // Check 6: Field Scope Violation (patch must target the EXACT field of the task)
        if (patch.field != targetField->field) {
            item.reasonCode = "FIELD_SCOPE_VIOLATION";
            item.reasonMessage = "Field scope violation: task '" + patch.taskId + "' is for field '" +
                                 targetField->field + "', but patch attempted to modify field '" + patch.field + "'.";
            report.rejectedPatches.push_back(std::move(item));
            continue;
        }

        // Check 7: Field Eligibility & Precedence (Cannot overwrite deterministic or OCR confident fields)
        if (targetField->resolutionStatus == ResolutionStatus::Deterministic ||
            targetField->resolutionStatus == ResolutionStatus::OcrConfident ||
            targetField->correction.applied) {
            item.reasonCode = "UNAUTHORIZED_FIELD_OVERWRITE";
            item.reasonMessage = "Cannot overwrite deterministic or OCR-confident field '" + targetField->field + "'.";
            report.rejectedPatches.push_back(std::move(item));
            continue;
        }

        if (!targetField->final.llmRequired && !targetField->llmEligible) {
            item.reasonCode = "FIELD_NOT_ELIGIBLE";
            item.reasonMessage = "Field '" + targetField->field + "' is not eligible for LLM resolution.";
            report.rejectedPatches.push_back(std::move(item));
            continue;
        }

        // Check 8: Resolution Status
        if (patch.resolutionStatus != "RESOLVED" && patch.resolutionStatus != "UNRESOLVED" &&
            patch.resolutionStatus != "AMBIGUOUS" && patch.resolutionStatus != "REJECTED") {
            item.reasonCode = "INVALID_STATUS";
            item.reasonMessage = "Unsupported resolution status: " + patch.resolutionStatus;
            report.rejectedPatches.push_back(std::move(item));
            continue;
        }

        if (patch.resolutionStatus != "RESOLVED") {
            item.reasonCode = "STATUS_" + patch.resolutionStatus;
            item.reasonMessage = "Resolution status is " + patch.resolutionStatus + "; skipping value application.";
            report.rejectedPatches.push_back(std::move(item));
            continue;
        }

        // Check 8: Original Value Validation
        std::string trimmedPatchOrig = trim(patch.originalValue);
        std::string trimmedFieldOrig = trim(targetField->original.value);
        std::string trimmedFieldNorm = trim(targetField->final.value);

        if (!trimmedPatchOrig.empty() &&
            trimmedPatchOrig != trimmedFieldOrig &&
            trimmedPatchOrig != trimmedFieldNorm) {
            item.reasonCode = "ORIGINAL_VALUE_MISMATCH";
            item.reasonMessage = "Patch original_value '" + patch.originalValue +
                                 "' does not match target source value '" + targetField->original.value + "'.";
            report.rejectedPatches.push_back(std::move(item));
            continue;
        }

        // Check 9: Source Page Validation
        if (patch.source.page > 0 &&
            targetField->original.source.page > 0 &&
            patch.source.page != targetField->original.source.page) {
            item.reasonCode = "SOURCE_MISMATCH";
            item.reasonMessage = "Patch source page (" + std::to_string(patch.source.page) +
                                 ") does not match task source page (" +
                                 std::to_string(targetField->original.source.page) + ").";
            report.rejectedPatches.push_back(std::move(item));
            continue;
        }

        // Check 10: Confidence Sanity Check
        if (std::isnan(patch.llmConfidence) ||
            patch.llmConfidence < 0.0f ||
            patch.llmConfidence > 1.0f) {
            item.reasonCode = "INVALID_CONFIDENCE";
            item.reasonMessage = "LLM confidence value " + std::to_string(patch.llmConfidence) + " is outside [0.0, 1.0].";
            report.rejectedPatches.push_back(std::move(item));
            continue;
        }

        // Check 11: Empty Correction
        if (trim(patch.correctedValue).empty()) {
            item.reasonCode = "EMPTY_CORRECTION";
            item.reasonMessage = "Patch corrected_value cannot be empty.";
            report.rejectedPatches.push_back(std::move(item));
            continue;
        }

        // Check 12: Evidence Supported
        if (!patch.evidenceSupported) {
            item.reasonCode = "EVIDENCE_NOT_SUPPORTED";
            item.reasonMessage = "Patch marked as not supported by evidence.";
            report.rejectedPatches.push_back(std::move(item));
            continue;
        }

        // Check 13: Conflicting Patches for same task
        auto taskAcceptedIt = taskAcceptedMap.find(patch.taskId);
        if (taskAcceptedIt != taskAcceptedMap.end()) {
            if (taskAcceptedIt->second.first == patch.correctedValue) {
                // Exact duplicate of previously accepted patch
                item.reasonCode = "DUPLICATE_PATCH";
                item.reasonMessage = "Duplicate patch for task " + patch.taskId + " with identical corrected value.";
                report.rejectedPatches.push_back(std::move(item));
                continue;
            } else {
                // Conflicting patch! Invalidate previous accepted patch and reject this one
                size_t prevIdx = taskAcceptedIt->second.second;
                if (prevIdx < report.acceptedPatches.size()) {
                    auto prevPatch = report.acceptedPatches[prevIdx];
                    prevPatch.status = PatchValidationStatus::Rejected;
                    prevPatch.reasonCode = "CONFLICTING_PATCHES";
                    prevPatch.reasonMessage = "Conflicting patch values proposed for task " + patch.taskId;
                    report.rejectedPatches.push_back(std::move(prevPatch));
                    // Remove from accepted
                    report.acceptedPatches.erase(report.acceptedPatches.begin() + prevIdx);
                }

                item.reasonCode = "CONFLICTING_PATCHES";
                item.reasonMessage = "Conflicting patch value '" + patch.correctedValue +
                                     "' differs from previously proposed value for task " + patch.taskId;
                report.rejectedPatches.push_back(std::move(item));
                continue;
            }
        }

        // All checks passed -> ACCEPT patch
        item.status = PatchValidationStatus::Accepted;
        item.reasonCode = patch.reasonCode.empty() ? "LLM_CORRECTION" : patch.reasonCode;
        item.reasonMessage = "Validated and accepted.";
        size_t accIdx = report.acceptedPatches.size();
        taskAcceptedMap[patch.taskId] = {patch.correctedValue, accIdx};
        report.acceptedPatches.push_back(std::move(item));
    }

    report.accepted = static_cast<int>(report.acceptedPatches.size());
    report.rejected = static_cast<int>(report.rejectedPatches.size());
    return report;
}

} // namespace ocr::evidence

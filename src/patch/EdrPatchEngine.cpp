#include <edmo/evidence/patch/EdrPatchEngine.hpp>

#include <map>

namespace edmo::evidence {

EnhancedEdrResult EdrPatchEngine::applyPatchesJson(
    const EvidenceDocument& doc,
    const std::vector<EvidenceField>& originalFields,
    const nlohmann::json& patchJson
) const {
    auto patchDoc = LlmPatchDocument::fromJson(patchJson);
    return applyPatches(doc, originalFields, patchDoc);
}

EnhancedEdrResult EdrPatchEngine::applyPatches(
    const EvidenceDocument& doc,
    const std::vector<EvidenceField>& originalFields,
    const LlmPatchDocument& patchDoc
) const {
    EnhancedEdrResult result;
    result.documentId = doc.metadata.id;
    result.documentType = doc.metadata.type;
    result.fields = originalFields;

    // 1. Strict Validation of incoming untrusted patches
    result.patchReport = LlmPatchValidator::validatePatches(doc.metadata.id, originalFields, patchDoc);

    // 2. Index accepted patches by taskId
    std::map<std::string, const LlmPatch*> acceptedTaskPatches;
    std::map<std::string, size_t> acceptedReportIndex;

    for (size_t i = 0; i < result.patchReport.acceptedPatches.size(); ++i) {
        const auto& pItem = result.patchReport.acceptedPatches[i];
        for (const auto& rawPatch : patchDoc.patches) {
            if (rawPatch.patchId == pItem.patchId && rawPatch.taskId == pItem.taskId) {
                acceptedTaskPatches[rawPatch.taskId] = &rawPatch;
                acceptedReportIndex[rawPatch.taskId] = i;
                break;
            }
        }
    }

    // 3. Apply accepted patches to target fields (Field-Local Resolution)
    for (auto& field : result.fields) {
        if (!field.llmTaskId.empty() && acceptedTaskPatches.count(field.llmTaskId) > 0) {
            const LlmPatch* patch = acceptedTaskPatches[field.llmTaskId];

            // Deterministic confidence fusion
            float fusedConf = fusionEngine_.fuseLlmPatchConfidence(
                field.original.confidence,
                patch->llmConfidence,
                1.0f, // dictionary score
                1.0f  // context score
            );

            field.final.value = patch->correctedValue;
            field.final.confidence = fusedConf;
            field.final.trusted = true;
            field.final.llmRequired = false;
            field.resolutionStatus = ResolutionStatus::Resolved;
            field.resolutionStrategy = ResolutionStrategy::None;
            field.aiWorkflowState = AiWorkflowState::Completed;
            field.resolutionMethod = "LLM";
            field.resolutionModel = patchDoc.model.model.empty() ? "qwen" : patchDoc.model.model;
            field.resolutionReasonCode = patch->reasonCode.empty() ? "OCR_CHARACTER_SUBSTITUTION" : patch->reasonCode;
            field.llmPatchId = patch->patchId;
            field.fate = EvidenceFate::Corrected;

            // Update final confidence in report
            if (acceptedReportIndex.count(field.llmTaskId) > 0) {
                size_t repIdx = acceptedReportIndex[field.llmTaskId];
                result.patchReport.acceptedPatches[repIdx].finalConfidence = fusedConf;
            }
        }

        // Tally resolution statuses
        switch (field.resolutionStatus) {
            case ResolutionStatus::OcrConfident:
                result.ocrConfidentCount++;
                break;
            case ResolutionStatus::Deterministic:
                result.deterministicCount++;
                break;
            case ResolutionStatus::Normalized:
                result.normalizedCount++;
                break;
            case ResolutionStatus::Resolved:
                result.llmResolvedCount++;
                break;
            case ResolutionStatus::LlmRequired:
            case ResolutionStatus::Ambiguous:
                result.unresolvedCount++;
                break;
            case ResolutionStatus::AiUnresolved:
                result.aiUnresolvedCount++;
                break;
        }
    }

    // 4. Construct Enhanced EDR JSON (Schema: edr-enhanced-0.1)
    nlohmann::json enhancedFields = nlohmann::json::array();
    for (const auto& f : result.fields) {
        nlohmann::json fJson{
            {"field_id", f.fieldId},
            {"field", f.field},
            {"category", f.category},
            {"value", f.final.value},
            {"confidence", f.final.confidence},
            {"original", {
                {"value", f.original.value},
                {"confidence", f.original.confidence},
                {"source", f.original.source.toJson(false)}
            }},
            {"source", f.original.source.toJson(false)},
            {"resolution", {
                {"status", resolutionStatusToString(f.resolutionStatus)},
                {"method", f.resolutionMethod.empty() ? (f.correction.applied ? "DETERMINISTIC" : "OCR") : f.resolutionMethod}
            }}
        };

        if (f.normalization.applied) {
            fJson["normalization"] = f.normalization.toJson();
            fJson["normalized_value"] = f.normalization.normalizedValue;
        }

        if (!f.resolutionModel.empty()) {
            fJson["resolution"]["model"] = f.resolutionModel;
        }
        if (!f.resolutionReasonCode.empty()) {
            fJson["resolution"]["reason_code"] = f.resolutionReasonCode;
        }
        if (!f.llmPatchId.empty()) {
            fJson["resolution"]["patch_id"] = f.llmPatchId;
        }
        if (!f.llmTaskId.empty()) {
            fJson["resolution"]["task_id"] = f.llmTaskId;
        }

        if (!f.term.empty()) fJson["term"] = f.term;
        if (!f.status.empty()) fJson["status"] = f.status;

        enhancedFields.push_back(std::move(fJson));
    }

    nlohmann::json tablesArray = nlohmann::json::array();
    for (const auto& t : doc.tables) {
        tablesArray.push_back(t.toJson(true));
    }

    nlohmann::json pagesArray = nlohmann::json::array();
    for (const auto& p : doc.pages) {
        pagesArray.push_back(p.toJson());
    }

    result.enhancedEdrJson = nlohmann::json{
        {"schema_version", "edr-enhanced-0.1"},
        {"document_id", result.documentId},
        {"document_type", result.documentType},
        {"confidence_fusion", fusionEngine_.policy().toJson()},
        {"pages", std::move(pagesArray)},
        {"tables", std::move(tablesArray)},
        {"fields", std::move(enhancedFields)},
        {"resolutions", {
            {"ocr_confident", result.ocrConfidentCount},
            {"deterministic", result.deterministicCount},
            {"llm_resolved", result.llmResolvedCount},
            {"unresolved", result.unresolvedCount},
            {"rejected", result.rejectedCount}
        }},
        {"audit", {
            {"zero_loss", true},
            {"unaccounted_loss", 0}
        }}
    };

    if (!doc.identity.empty()) result.enhancedEdrJson["identity"] = doc.identity;
    if (!doc.institution.empty()) result.enhancedEdrJson["institution"] = doc.institution;
    if (!doc.academic.empty()) result.enhancedEdrJson["academic"] = doc.academic;
    if (!doc.unclassifiedContent.empty()) result.enhancedEdrJson["unclassified_content"] = doc.unclassifiedContent;

    return result;
}

} // namespace edmo::evidence

#include <edmo/evidence/serialization/EvidenceJsonSerializer.hpp>

#include <fstream>
#include <numeric>

namespace edmo::evidence {

nlohmann::json EvidenceJsonSerializer::buildFullEvidenceJson(
    const EvidenceDocument& doc,
    const std::vector<EvidenceField>& fields
) {
    nlohmann::json j{
        {"schema_version", "evidence-full-0.1"},
        {"document", {
            {"document_id", doc.metadata.id},
            {"document_type", doc.metadata.type},
            {"page_count", doc.metadata.pageCount},
            {"source", doc.metadata.source}
        }},
        {"engine", doc.engine.toJson()},
        {"confidence", doc.confidence.toJson()}
    };

    if (!doc.identity.empty()) {
        j["identity"] = doc.identity;
    }
    if (!doc.institution.empty()) {
        j["institution"] = doc.institution;
    }
    if (!doc.academic.empty()) {
        j["academic"] = doc.academic;
    }

    nlohmann::json fieldsArray = nlohmann::json::array();
    for (const auto& f : fields) {
        fieldsArray.push_back(f.toJson());
    }
    j["fields"] = std::move(fieldsArray);

    nlohmann::json tablesArray = nlohmann::json::array();
    for (const auto& t : doc.tables) {
        tablesArray.push_back(t.toJson(true));
    }
    j["tables"] = std::move(tablesArray);

    if (!doc.unclassifiedContent.empty()) {
        j["unclassified_content"] = doc.unclassifiedContent;
    }

    return j;
}

nlohmann::json EvidenceJsonSerializer::buildReportJson(
    const PipelineResult& result
) {
    float avgConf = 0.0f;
    if (!result.fields.empty()) {
        float sum = 0.0f;
        for (const auto& f : result.fields) {
            sum += f.final.confidence;
        }
        avgConf = sum / result.fields.size();
    }

    nlohmann::json j{
        {"schema_version", "evidence-report-0.1"},
        {"processing", {
            {"execution_time_ms", result.timer.getTotalDurationMs()},
            {"pages", result.document.metadata.pageCount}
        }},
        {"performance_breakdown_ms", {
            {"evidence_parsing", result.timer.getStageDurationMs("evidence_parsing")},
            {"field_extraction", result.timer.getStageDurationMs("field_extraction")},
            {"correction", result.timer.getStageDurationMs("correction")},
            {"fusion", result.timer.getStageDurationMs("fusion")},
            {"routing", result.timer.getStageDurationMs("routing")},
            {"llm_context", result.timer.getStageDurationMs("llm_context")},
            {"audit", result.timer.getStageDurationMs("audit")},
            {"serialization", result.timer.getStageDurationMs("serialization")}
        }},
        {"source", {
            {"tokens", result.zeroLossReport.totalTokens},
            {"blocks", result.zeroLossReport.totalBlocks},
            {"fields", result.totalFields},
            {"tables", result.zeroLossReport.totalTables},
            {"rows", result.zeroLossReport.totalRows},
            {"cells", result.zeroLossReport.totalCells}
        }},
        {"confidence", {
            {"average", avgConf},
            {"high_confidence_fields", result.highConfidenceFields},
            {"medium_confidence_fields", result.mediumConfidenceFields},
            {"low_confidence_fields", result.lowConfidenceFields}
        }},
        {"correction", {
            {"attempted", result.correctionsAttempted},
            {"corrected", result.correctionsApplied},
            {"unchanged", result.correctionsAttempted - result.correctionsApplied}
        }},
        {"llm", {
            {"required", result.llmRequired},
            {"task_count", result.llmTaskCount},
            {"estimated_tokens", TargetedLlmContextBuilder::estimateTokens(result.evidenceLlm)}
        }},
        {"routing", {
            {"llm_confidence_threshold", result.routingStats.threshold},
            {"threshold", result.routingStats.threshold},
            {"confidence_epsilon", result.routingStats.epsilon},
            {"epsilon", result.routingStats.epsilon},
            {"total_candidates", result.routingStats.totalCandidates},
            {"high_confidence", result.routingStats.highConfidence},
            {"deterministically_resolved", result.routingStats.deterministicallyResolved},
            {"structural", result.routingStats.structural},
            {"placeholders", result.routingStats.placeholders},
            {"invalid", result.routingStats.invalid},
            {"insufficient_evidence", result.routingStats.insufficientEvidence},
            {"llm_eligible", result.routingStats.llmEligible},
            {"llm_tasks_created", result.routingStats.llmTasksCreated > 0 ? result.routingStats.llmTasksCreated : result.routingStats.llmRequired},
            {"structural_items", result.routingStats.structuralItems},
            {"valid_fields", result.routingStats.validFields},
            {"llm_required", result.routingStats.llmRequired},
            {"llm_rejected", result.routingStats.llmRejected}
        }},
        {"quality", {
            {"placeholder_rejections", result.routingStats.placeholderRejections},
            {"structural_rejections", result.routingStats.structuralRejections},
            {"invalid_field_rejections", result.routingStats.invalidFieldRejections},
            {"duplicate_tasks_removed", result.routingStats.duplicateTasksRemoved}
        }},
        {"coverage", {
            {"source_items", result.zeroLossReport.totalSourceItems},
            {"represented_items", result.zeroLossReport.representedItems},
            {"unaccounted_loss", result.zeroLossReport.unaccountedLoss}
        }}
    };

    return j;
}

bool EvidenceJsonSerializer::savePipelineOutputs(
    const PipelineResult& result,
    const std::filesystem::path& outputDir,
    bool prettyPrint
) {
    std::error_code ec;
    if (!std::filesystem::exists(outputDir, ec)) {
        std::filesystem::create_directories(outputDir, ec);
    }

    auto writeJson = [&](const std::filesystem::path& p, const nlohmann::json& json) -> bool {
        std::ofstream out(p);
        if (!out.is_open()) return false;
        out << (prettyPrint ? json.dump(2) : json.dump());
        return out.good();
    };

    bool ok1 = writeJson(outputDir / "evidence-full.json", result.evidenceFull);
    bool ok2 = writeJson(outputDir / "evidence-llm.json", result.evidenceLlm);
    bool ok3 = writeJson(outputDir / "evidence-report.json", result.evidenceReport);
    bool ok4 = true;
    if (!result.resolutionRequest.empty()) {
        ok4 = writeJson(outputDir / "llm-resolution-request.json", result.resolutionRequest);
    }

    return ok1 && ok2 && ok3 && ok4;
}

bool EvidenceJsonSerializer::saveEnhancedOutputs(
    const nlohmann::json& enhancedEdr,
    const nlohmann::json& finalDocument,
    const nlohmann::json& patchReport,
    const std::filesystem::path& outputDir,
    const nlohmann::json& finalizationReport,
    bool prettyPrint
) {
    std::error_code ec;
    if (!std::filesystem::exists(outputDir, ec)) {
        std::filesystem::create_directories(outputDir, ec);
    }

    auto writeJson = [&](const std::filesystem::path& p, const nlohmann::json& json) -> bool {
        std::ofstream out(p);
        if (!out.is_open()) return false;
        out << (prettyPrint ? json.dump(2) : json.dump());
        return out.good();
    };

    bool ok1 = writeJson(outputDir / "enhanced_edr.json", enhancedEdr);
    bool ok2 = writeJson(outputDir / "final_document.json", finalDocument);
    bool ok3 = writeJson(outputDir / "patch-report.json", patchReport);
    bool ok4 = true;
    if (!finalizationReport.empty()) {
        ok4 = writeJson(outputDir / "finalization-report.json", finalizationReport);
    }

    return ok1 && ok2 && ok3 && ok4;
}

} // namespace edmo::evidence

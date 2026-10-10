#include <ocr/evidence/routing/ConfidenceRouter.hpp>
#include <ocr/evidence/resolution/ResolutionAnalyzer.hpp>

#include <set>
#include <tuple>

namespace ocr::evidence {

RoutingDecision ConfidenceRouter::routeField(
    const EvidenceField& field
) const noexcept {
    RoutingDecision decision;
    decision.llmDecision.field = field.field;
    decision.llmDecision.confidence = field.final.confidence;
    decision.llmDecision.threshold = config_.highThreshold;
    decision.llmDecision.semanticValid = field.isSemanticValid;
    decision.llmDecision.deterministicallyResolved = field.correction.applied;
    decision.llmDecision.evidenceSufficient = !field.original.value.empty();

    ResolutionAnalyzer analyzer(config_);
    EvidenceDocument dummyDoc;
    std::vector<EvidenceField> dummyList = {field};
    auto analysis = analyzer.analyzeField(field, dummyDoc, dummyList);

    decision.isTrusted = analysis.isTrusted;
    decision.llmRequired = analysis.llmRequired;
    decision.llmEligible = analysis.llmRequired;
    decision.resolutionStatus = analysis.status;
    decision.reason = analysis.reasonMessage;
    decision.llmDecision.eligible = analysis.llmRequired;
    decision.llmDecision.reasonCode = analysis.reasonCode;

    if (analysis.status == ResolutionStatus::Deterministic) {
        decision.fate = field.correction.applied ? EvidenceFate::Corrected : EvidenceFate::Preserved;
        decision.llmDecision.deterministicallyResolved = true;
    } else if (analysis.status == ResolutionStatus::OcrConfident || analysis.status == ResolutionStatus::Normalized) {
        decision.fate = EvidenceFate::Preserved;
    } else if (analysis.status == ResolutionStatus::LlmRequired) {
        decision.fate = EvidenceFate::LlmContext;
    } else if (field.structuralClass == StructuralClass::Placeholder || field.structuralClass == StructuralClass::Noise) {
        decision.fate = EvidenceFate::Noise;
    } else {
        decision.fate = EvidenceFate::Preserved;
    }

    return decision;
}

RoutingStats ConfidenceRouter::routeFields(std::vector<EvidenceField>& fields) const noexcept {
    EvidenceDocument dummyDoc;
    return routeFields(fields, dummyDoc);
}

RoutingStats ConfidenceRouter::routeFields(
    std::vector<EvidenceField>& fields,
    const EvidenceDocument& doc
) const noexcept {
    RoutingStats stats;
    stats.threshold = config_.highThreshold;
    stats.epsilon = config_.epsilon;

    std::set<std::tuple<std::string, int, std::string>> seenTasks;
    int currentLlmTaskCount = 0;

    ResolutionAnalyzer analyzer(config_);

    for (auto& field : fields) {
        stats.totalCandidates++;

        if (field.structuralClass == StructuralClass::Placeholder) {
            stats.placeholders++;
            stats.placeholderRejections++;
            stats.structuralItems++;
        } else if (field.structuralClass == StructuralClass::Noise) {
            stats.noise++;
            stats.structuralRejections++;
            stats.structuralItems++;
        } else if (isStructuralOrNonSemantic(field.structuralClass)) {
            stats.structural++;
            stats.structuralRejections++;
            stats.structuralItems++;
        } else if (!field.isSemanticValid) {
            stats.invalid++;
            stats.invalidFieldRejections++;
        } else {
            stats.validFields++;
        }

        auto analysis = analyzer.analyzeField(field, doc, fields);
        field.resolutionStatus = analysis.status;
        field.resolutionStrategy = analysis.strategy;
        field.problem = analysis.problem;
        field.final.trusted = analysis.isTrusted;
        field.final.llmRequired = analysis.llmRequired;
        field.llmEligible = analysis.llmRequired;
        field.llmReason = analysis.reasonCode;

        if (analysis.status == ResolutionStatus::Deterministic) {
            field.resolutionMethod = "DETERMINISTIC";
            field.fate = field.correction.applied ? EvidenceFate::Corrected : EvidenceFate::Preserved;
            stats.deterministicallyResolved++;
        } else if (analysis.status == ResolutionStatus::OcrConfident) {
            field.resolutionMethod = "OCR";
            field.fate = EvidenceFate::Preserved;
            stats.highConfidence++;
        } else if (analysis.status == ResolutionStatus::Normalized) {
            field.resolutionMethod = "NORMALIZED";
            field.fate = EvidenceFate::Preserved;
            stats.highConfidence++;
        } else if (analysis.status == ResolutionStatus::LlmRequired) {
            field.resolutionMethod = "LLM_REQUIRED";
            field.fate = EvidenceFate::LlmContext;
            stats.llmEligible++;

            // Deduplication and budget enforcement
            auto taskKey = std::make_tuple(
                field.field,
                field.original.source.page,
                field.final.value
            );

            if (config_.enableDeduplication && seenTasks.count(taskKey) > 0) {
                field.final.llmRequired = false;
                field.fate = EvidenceFate::Deduplicated;
                field.llmReason = "DUPLICATE_TASK";
                stats.duplicateTasksRemoved++;
                stats.llmRejected++;
            } else if (currentLlmTaskCount >= config_.maxLlmTasks) {
                field.final.llmRequired = false;
                field.fate = EvidenceFate::Chunked;
                field.llmReason = "LLM_TASK_BUDGET_EXCEEDED";
                stats.llmRejected++;
            } else {
                seenTasks.insert(taskKey);
                currentLlmTaskCount++;
                stats.llmRequired++;
                stats.llmTasksCreated++;
            }
        } else {
            field.resolutionMethod = "AI_UNRESOLVED";
            stats.llmRejected++;
            if (analysis.reasonCode == "INSUFFICIENT_EVIDENCE") {
                stats.insufficientEvidence++;
            }
        }
    }

    return stats;
}

} // namespace ocr::evidence

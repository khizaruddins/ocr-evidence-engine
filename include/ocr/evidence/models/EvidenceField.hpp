#pragma once

#include <ocr/evidence/models/EvidenceSource.hpp>
#include <ocr/evidence/models/Fates.hpp>
#include <ocr/evidence/structure/StructuralTypes.hpp>
#include <ocr/evidence/normalization/FieldValueNormalizer.hpp>
#include <ocr/evidence/resolution/ResolutionProblem.hpp>
#include <ocr/evidence/resolution/ResolutionStrategy.hpp>

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

/**
 * @brief Candidate correction generated during deterministic OCR correction.
 */
struct CorrectionCandidate {
    std::string value;
    CorrectionMethod method = CorrectionMethod::None;
    float confidence = 0.0f;
    float similarityScore = 0.0f;
    float dictionaryScore = 0.0f;
    float contextScore = 0.0f;
    float structuralScore = 0.0f;
    std::string reason;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"value", value},
            {"method", correctionMethodToString(method)},
            {"confidence", confidence},
            {"similarity_score", similarityScore},
            {"dictionary_score", dictionaryScore},
            {"context_score", contextScore},
            {"reason", reason}
        };
    }
};

/**
 * @brief Original raw OCR evidence preservation struct.
 */
struct OriginalEvidence {
    std::string value;
    float confidence = 1.0f;
    EvidenceSource source;
    std::vector<std::string> rawTokens;
    int lowConfidenceTokenCount = 0;
    float minConfidence = 1.0f;
    float meanConfidence = 1.0f;
    float maxConfidence = 1.0f;

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json j{
            {"value", value},
            {"confidence", confidence},
            {"source", source.toJson()}
        };
        if (!rawTokens.empty()) {
            j["raw_tokens"] = rawTokens;
        }
        if (lowConfidenceTokenCount > 0) {
            j["low_confidence_tokens"] = lowConfidenceTokenCount;
        }
        return j;
    }
};

/**
 * @brief Correction details struct.
 */
struct CorrectionResult {
    std::string value;
    CorrectionMethod method = CorrectionMethod::None;
    float confidence = 0.0f;
    bool applied = false;
    std::vector<CorrectionCandidate> candidates;
    std::string provenance;

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json j{
            {"value", value},
            {"method", correctionMethodToString(method)},
            {"confidence", confidence},
            {"applied", applied}
        };
        if (!candidates.empty()) {
            nlohmann::json candArray = nlohmann::json::array();
            for (const auto& c : candidates) {
                candArray.push_back(c.toJson());
            }
            j["candidates"] = std::move(candArray);
        }
        if (!provenance.empty()) {
            j["provenance"] = provenance;
        }
        return j;
    }
};

/**
 * @brief Evidence fusion breakdown.
 */
struct FusionResult {
    float originalConfidence = 1.0f;
    float dictionaryScore = 0.0f;
    float fuzzyScore = 0.0f;
    float contextScore = 0.0f;
    float structuralScore = 1.0f;
    float multiHypothesisScore = 1.0f;
    float finalConfidence = 1.0f;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"original_confidence", originalConfidence},
            {"dictionary_score", dictionaryScore},
            {"fuzzy_score", fuzzyScore},
            {"context_score", contextScore},
            {"structural_score", structuralScore},
            {"final_confidence", finalConfidence}
        };
    }
};

/**
 * @brief Final resolution struct.
 */
struct FinalResolution {
    std::string value;
    float confidence = 1.0f;
    bool trusted = true;
    bool llmRequired = false;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"value", value},
            {"confidence", confidence},
            {"trusted", trusted},
            {"llm_required", llmRequired}
        };
    }
};

/**
 * @brief Enriched evidence field representing a normalized semantic entity.
 */
struct EvidenceField {
    std::string fieldId;
    std::string field;
    std::string category = "general"; // identity, institution, academic, course_record, summary, unclassified

    OriginalEvidence original;
    NormalizationResult normalization;
    CorrectionResult correction;
    FusionResult fusion;
    FinalResolution final;

    StructuralClass structuralClass = StructuralClass::Unknown;
    ResolutionStatus resolutionStatus = ResolutionStatus::AiUnresolved;
    ResolutionStrategy resolutionStrategy = ResolutionStrategy::None;
    AiWorkflowState aiWorkflowState = AiWorkflowState::NotStarted;
    int aiAttemptCount = 0;

    ResolutionProblem problem;

    std::string sourceBlockId;
    std::string labelBlockId;
    std::string labelText;
    std::string semanticRelationship = "VALUE_FOR_LABEL";
    std::vector<std::string> conflictingCandidates;

    bool isSemanticValid = true;
    bool llmEligible = true;
    std::string llmReason;
    std::string llmTaskId;
    std::string llmPatchId;
    std::string resolutionMethod;
    std::string resolutionModel;
    std::string resolutionReasonCode;

    std::string term;
    std::string status;

    EvidenceFate fate = EvidenceFate::Preserved;
    std::vector<std::string> nearbyEvidence;

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json j{
            {"field_id", fieldId},
            {"field", field},
            {"category", category},
            {"original", original.toJson()},
            {"fate", fateToString(fate)},
            {"structural_class", structuralClassToString(structuralClass)},
            {"resolution_status", resolutionStatusToString(resolutionStatus)},
            {"resolution_strategy", resolutionStrategyToString(resolutionStrategy)},
            {"ai_workflow_state", aiWorkflowStateToString(aiWorkflowState)},
            {"ai_attempt_count", aiAttemptCount},
            {"is_semantic_valid", isSemanticValid}
        };

        if (!problem.empty()) {
            j["problem"] = problem.toJson();
        }

        if (!sourceBlockId.empty()) {
            j["source_block_id"] = sourceBlockId;
        }
        if (!labelBlockId.empty()) {
            j["label_block_id"] = labelBlockId;
        }
        if (!labelText.empty()) {
            j["label_text"] = labelText;
        }
        if (!semanticRelationship.empty()) {
            j["semantic_relationship"] = semanticRelationship;
        }
        if (!conflictingCandidates.empty()) {
            j["conflicting_candidates"] = conflictingCandidates;
        }

        if (normalization.applied || !normalization.operations.empty()) {
            j["normalization"] = normalization.toJson();
            j["normalized_value"] = normalization.normalizedValue;
        }

        if (!llmTaskId.empty()) {
            j["llm_task_id"] = llmTaskId;
        }
        if (!llmPatchId.empty()) {
            j["llm_patch_id"] = llmPatchId;
        }

        if (!term.empty()) {
            j["term"] = term;
        }
        if (!status.empty()) {
            j["status"] = status;
        }

        if (correction.applied || !correction.value.empty()) {
            j["correction"] = correction.toJson();
        }

        j["fusion"] = fusion.toJson();
        j["final"] = final.toJson();

        if (!resolutionMethod.empty()) {
            nlohmann::json resObj{
                {"status", resolutionStatusToString(resolutionStatus)},
                {"method", resolutionMethod}
            };
            if (!resolutionModel.empty()) resObj["model"] = resolutionModel;
            if (!resolutionReasonCode.empty()) resObj["reason_code"] = resolutionReasonCode;
            j["resolution"] = std::move(resObj);
        }

        if (!nearbyEvidence.empty()) {
            j["nearby_evidence"] = nearbyEvidence;
        }

        return j;
    }
};

} // namespace ocr::evidence

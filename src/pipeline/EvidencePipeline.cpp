#include <ocr/evidence/pipeline/EvidencePipeline.hpp>
#include <ocr/evidence/serialization/EvidenceJsonSerializer.hpp>

#include <fstream>
#include <iostream>
#include <regex>

namespace ocr::evidence {

EvidencePipeline::EvidencePipeline(PipelineConfig config)
    : config_(config) {
    initializeComponents();
}

void EvidencePipeline::setConfig(const PipelineConfig& config) {
    config_ = config;
    initializeComponents();
}

void EvidencePipeline::initializeComponents() {
    dictionary_ = std::make_shared<DictionaryEngine>();
    if (!config_.dictionariesDir.empty() && std::filesystem::exists(config_.dictionariesDir)) {
        dictionary_->loadDictionariesFromDir(config_.dictionariesDir);
    }

    confusion_ = std::make_shared<CharacterConfusionMatrix>();
    if (!config_.confusionFile.empty() && std::filesystem::exists(config_.confusionFile)) {
        confusion_->loadFromFile(config_.confusionFile);
    }

    correctionEngine_ = std::make_shared<CorrectionEngine>(confusion_, dictionary_);
    contextualCorrectionEngine_ = std::make_shared<ContextualCorrectionEngine>(correctionEngine_);
    confidenceEngine_ = std::make_shared<ConfidenceEngine>(config_.confidenceConfig);
    fusionEngine_ = std::make_shared<EvidenceFusionEngine>(confidenceEngine_);
    router_ = std::make_shared<ConfidenceRouter>(config_.routerConfig);
    transcriptExtractor_ = std::make_shared<TranscriptFieldExtractor>();
    candidateValidator_ = std::make_shared<FieldCandidateValidator>();
}

PipelineResult EvidencePipeline::processFile(const std::filesystem::path& edrFilePath) {
    if (!std::filesystem::exists(edrFilePath)) {
        throw std::runtime_error("Full EDR file does not exist: " + edrFilePath.string());
    }
    std::ifstream in(edrFilePath);
    if (!in.is_open()) {
        throw std::runtime_error("Cannot open Full EDR file: " + edrFilePath.string());
    }
    nlohmann::json j;
    in >> j;
    return process(j);
}

PipelineResult EvidencePipeline::process(const nlohmann::json& fullEdrJson) {
    PipelineResult result;

    // Stage 1: Evidence Parsing
    result.timer.startStage("evidence_parsing");
    result.document = EdrParser::parseJson(fullEdrJson);
    if (!config_.documentId.empty()) {
        result.document.metadata.id = config_.documentId;
    }
    result.timer.stopStage();

    // Stage 2: Field Extraction (Integrated with Structural Classification & Course Row Detection)
    result.timer.startStage("field_extraction");
    result.fields = transcriptExtractor_->extractFields(result.document);
    result.timer.stopStage();

    // Stage 3: Field Candidate Validation Gate
    result.timer.startStage("candidate_validation");
    candidateValidator_->validateFields(result.fields);
    result.timer.stopStage();

    // Stage 4: Deterministic OCR Correction (Only for semantically valid candidates)
    result.timer.startStage("correction");
    if (config_.enableCorrection) {
        for (auto& field : result.fields) {
            if (!field.isSemanticValid || isStructuralOrNonSemantic(field.structuralClass)) {
                continue;
            }

            result.correctionsAttempted++;
            auto corr = correctionEngine_->correctText(
                field.original.value,
                field.field,
                field.original.confidence
            );

            if (corr.applied && corr.value != field.original.value) {
                field.correction = corr;
                field.final.value = corr.value;
                field.final.confidence = corr.confidence;
                field.fate = EvidenceFate::Corrected;
                result.correctionsApplied++;
            } else {
                field.correction = corr;
            }
        }

        if (config_.enableContextualCorrection) {
            contextualCorrectionEngine_->applyContextualCorrections(result.fields);
        }
    }
    result.timer.stopStage();

    // Stage 5: Evidence Fusion
    result.timer.startStage("fusion");
    if (config_.enableFusion) {
        static const std::regex codePattern(R"(^(?:[A-Z]{2,5}[-\s]?[0-9]{2,4}[A-Z]?|[0-9]{2}[A-Z]{2,4}[0-9]{2,4}[A-Z]?)$)", std::regex::icase);
        static const std::regex creditsPattern(R"(^[0-9]+(?:\.[0-9]+)?$)");

        for (auto& field : result.fields) {
            if (!field.isSemanticValid || isStructuralOrNonSemantic(field.structuralClass)) {
                if (field.structuralClass == StructuralClass::TableSeparator ||
                    field.structuralClass == StructuralClass::Placeholder) {
                    field.fusion.finalConfidence = 0.0f;
                    field.final.confidence = 0.0f;
                }
                continue;
            }

            float dictScore = 0.0f;
            if (field.field == "course_code") {
                dictScore = std::regex_match(field.final.value, codePattern) ? 1.0f : 0.4f;
            } else if (field.field == "credits" || field.field == "quality_points") {
                dictScore = std::regex_match(field.final.value, creditsPattern) ? 1.0f : 0.0f;
            } else if (field.field == "grade") {
                dictScore = dictionary_->isValidGrade(field.final.value) ? 1.0f : 0.0f;
            } else if (field.field == "student_id") {
                dictScore = 1.0f;
            } else if (field.field == "course_title") {
                dictScore = dictionary_->scoreTerm(field.final.value);
                if (dictScore < 0.85f) dictScore = 0.90f;
            } else {
                dictScore = dictionary_->scoreTerm(field.final.value);
                if (dictScore == 0.0f && (field.category == "identity" || field.category == "academic" || field.category == "institution")) {
                    dictScore = 0.95f; // Proper nouns for names, academic programs, and institution names
                }
            }

            float fuzzyScore = 1.0f;
            float ctxScore = 1.0f;
            float structScore = (field.structuralClass == StructuralClass::CourseRow ||
                                 field.structuralClass == StructuralClass::StudentMetadata ||
                                 field.structuralClass == StructuralClass::InstitutionData) ? 1.0f : 0.8f;

            field.fusion = fusionEngine_->fuseField(
                field,
                dictScore,
                fuzzyScore,
                ctxScore,
                structScore
            );
            field.final.confidence = field.fusion.finalConfidence;
        }
    }
    result.timer.stopStage();

    // Stage 6: Confidence Routing & Deduplication
    result.timer.startStage("routing");
    if (config_.enableRouting) {
        result.routingStats = router_->routeFields(result.fields, result.document);
    }
    result.timer.stopStage();

    // Stage 7: Targeted LLM Context & Resolution Request Builder
    result.timer.startStage("llm_context");
    result.evidenceLlm = TargetedLlmContextBuilder::buildContext(result.document, result.fields, config_.contextLevel);
    result.resolutionRequest = TargetedLlmContextBuilder::buildResolutionRequest(result.document, result.fields, config_.contextLevel);
    result.llmRequired = (result.routingStats.llmRequired > 0);
    if (result.resolutionRequest.contains("tasks") && result.resolutionRequest["tasks"].is_array()) {
        result.llmTaskCount = static_cast<int>(result.resolutionRequest["tasks"].size());
        result.llmRequired = (result.llmTaskCount > 0);
    }
    result.timer.stopStage();

    // Stage 8: Zero-Loss Audit
    result.timer.startStage("audit");
    result.zeroLossReport = ZeroLossAuditor::audit(result.document, result.fields);
    result.timer.stopStage();

    // Stage 9: Output Serialization
    result.timer.startStage("serialization");
    result.evidenceFull = EvidenceJsonSerializer::buildFullEvidenceJson(result.document, result.fields);
    result.totalFields = static_cast<int>(result.fields.size());

    for (const auto& f : result.fields) {
        if (f.final.confidence >= config_.routerConfig.highThreshold) {
            result.highConfidenceFields++;
        } else if (f.final.confidence >= config_.routerConfig.mediumThreshold) {
            result.mediumConfidenceFields++;
        } else {
            result.lowConfidenceFields++;
        }
    }

    result.evidenceReport = EvidenceJsonSerializer::buildReportJson(result);
    result.timer.stopStage();

    return result;
}

} // namespace ocr::evidence

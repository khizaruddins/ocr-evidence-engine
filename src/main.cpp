#include <ocr/evidence/pipeline/EvidencePipeline.hpp>
#include <ocr/evidence/serialization/EvidenceJsonSerializer.hpp>
#include <ocr/evidence/patch/EdrPatchEngine.hpp>
#include <ocr/evidence/projection/FinalDocumentProjector.hpp>
#include <ocr/evidence/validation/DocumentIdentityValidator.hpp>
#include <ocr/evidence/validation/FinalDocumentValidator.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

void printHelp(const char* progName) {
    std::cout << "======================================================\n"
              << "  OCR Evidence Intelligence Engine v0.3 (CLI)\n"
              << "======================================================\n\n"
              << "Usage:\n"
              << "  " << progName << " --input <full-edr.json> [options]\n\n"
              << "Options:\n"
              << "  -i, --input <path>               Path to input Full EDR JSON file (required)\n"
              << "      --document-id <id>           Explicit canonical document ID\n"
              << "      --llm-patch <path>           Path to LLM patch JSON file for reverse-path merge\n"
              << "  -t, --document-type <type>       Document classification type (default: transcript)\n"
              << "  -o, --output-dir <dir>           Output directory for evidence JSON files (default: output)\n"
              << "      --output <file>              Custom path for primary evidence result\n"
              << "      --llm-threshold <val>        High confidence threshold for LLM routing (default: 0.90)\n"
              << "      --llm-confidence-threshold <val> Alias for --llm-threshold\n"
              << "      --confidence-epsilon <val>   Floating-point margin for confidence comparison (default: 0.001)\n"
              << "      --llm-max-tasks <N>          Maximum task budget for LLM context (default: 50)\n"
              << "      --medium-threshold <val>     Medium confidence threshold (default: 0.85)\n"
              << "  -d, --dictionaries <dir>         Path to directory containing custom JSON dictionaries\n"
              << "  -c, --confusion <path>           Path to custom OCR character confusions JSON\n"
              << "  -b, --benchmark                  Print execution benchmarks and per-stage latency breakdown\n"
              << "  -v, --verbose                    Print verbose field extraction and routing trace\n"
              << "      --debug                      Enable extra diagnostics\n"
              << "  -h, --help                       Display this help message and exit\n\n"
              << "Outputs Generated:\n"
              << "  Forward Path (Evidence Extraction & Routing):\n"
              << "    output/evidence-full.json      Enriched evidence representation preserving all original OCR\n"
              << "    output/evidence-llm.json       Minimal targeted context containing only unresolved fields\n"
              << "    output/evidence-report.json    Comprehensive confidence, routing, quality, and zero-loss report\n"
              << "  Reverse Path (When --llm-patch is supplied):\n"
              << "    output/enhanced_edr.json       Canonical enhanced evidence document (schema: edr-enhanced-0.1)\n"
              << "    output/final_document.json     Application-facing semantic projection JSON\n"
              << "    output/patch-report.json       Zero-loss LLM patch validation and merge audit report\n\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp(argv[0]);
        return 1;
    }

    std::string inputPath;
    std::string documentId;
    std::string llmPatchPath;
    std::string documentType = "transcript";
    std::filesystem::path outputDir = "output";
    std::filesystem::path singleOutputFile;
    std::filesystem::path dictionariesDir;
    std::filesystem::path confusionFile;
    float llmThreshold = 0.90f;
    float mediumThreshold = 0.85f;
    float confidenceEpsilon = 0.001f;
    int maxLlmTasks = 50;
    bool benchmark = false;
    bool verbose = false;
    bool debug = false;
    std::string contextLevelStr = "targeted";

    // Check environment variable
    const char* envThresh = std::getenv("OCR_LLM_CONFIDENCE_THRESHOLD");
    if (!envThresh) {
        envThresh = std::getenv("EDMO_LLM_CONFIDENCE_THRESHOLD");
    }
    if (envThresh) {
        try {
            llmThreshold = std::stof(envThresh);
        } catch (...) {}
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp(argv[0]);
            return 0;
        } else if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
            inputPath = argv[++i];
        } else if (arg == "--document-id" && i + 1 < argc) {
            documentId = argv[++i];
        } else if (arg == "--llm-patch" && i + 1 < argc) {
            llmPatchPath = argv[++i];
        } else if ((arg == "-t" || arg == "--document-type") && i + 1 < argc) {
            documentType = argv[++i];
        } else if ((arg == "-o" || arg == "--output-dir") && i + 1 < argc) {
            outputDir = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            singleOutputFile = argv[++i];
        } else if ((arg == "--llm-threshold" || arg == "--llm-confidence-threshold") && i + 1 < argc) {
            llmThreshold = std::stof(argv[++i]);
        } else if (arg == "--confidence-epsilon" && i + 1 < argc) {
            confidenceEpsilon = std::stof(argv[++i]);
        } else if (arg == "--llm-max-tasks" && i + 1 < argc) {
            maxLlmTasks = std::stoi(argv[++i]);
        } else if (arg == "--medium-threshold" && i + 1 < argc) {
            mediumThreshold = std::stof(argv[++i]);
        } else if ((arg == "-d" || arg == "--dictionaries") && i + 1 < argc) {
            dictionariesDir = argv[++i];
        } else if ((arg == "-c" || arg == "--confusion") && i + 1 < argc) {
            confusionFile = argv[++i];
        } else if (arg == "-b" || arg == "--benchmark") {
            benchmark = true;
        } else if (arg == "--context-level" && i + 1 < argc) {
            contextLevelStr = argv[++i];
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "--debug") {
            debug = true;
        }
    }

    if (inputPath.empty()) {
        std::cerr << "Error: Missing required --input argument.\n";
        printHelp(argv[0]);
        return 1;
    }

    // Auto-locate data directory relative to executable or current directory if not explicitly provided
    if (dictionariesDir.empty()) {
        std::vector<std::filesystem::path> candidates = {
            "data/dictionaries",
            "../data/dictionaries",
            "../../data/dictionaries",
            "ocr-evidence-engine/data/dictionaries"
        };
        for (const auto& cand : candidates) {
            if (std::filesystem::exists(cand)) {
                dictionariesDir = cand;
                break;
            }
        }
    }

    if (confusionFile.empty()) {
        std::vector<std::filesystem::path> candidates = {
            "data/confusion/ocr-character-confusions.json",
            "../data/confusion/ocr-character-confusions.json",
            "../../data/confusion/ocr-character-confusions.json",
            "ocr-evidence-engine/data/confusion/ocr-character-confusions.json"
        };
        for (const auto& cand : candidates) {
            if (std::filesystem::exists(cand)) {
                confusionFile = cand;
                break;
            }
        }
    }

    ocr::evidence::PipelineConfig config;
    config.documentId = documentId;
    config.documentType = documentType;
    config.dictionariesDir = dictionariesDir;
    config.confusionFile = confusionFile;
    config.routerConfig.highThreshold = llmThreshold;
    config.routerConfig.mediumThreshold = mediumThreshold;
    config.routerConfig.epsilon = confidenceEpsilon;
    config.routerConfig.maxLlmTasks = maxLlmTasks;
    config.verbose = verbose;
    config.debug = debug;
    if (contextLevelStr == "expanded" || contextLevelStr == "EXPANDED") {
        config.contextLevel = ocr::evidence::ContextLevel::Expanded;
    } else {
        config.contextLevel = ocr::evidence::ContextLevel::Targeted;
    }

    try {
        ocr::evidence::EvidencePipeline pipeline(config);
        auto result = pipeline.processFile(inputPath);

        // Save standard forward path outputs
        ocr::evidence::EvidenceJsonSerializer::savePipelineOutputs(result, outputDir, true);

        if (!singleOutputFile.empty()) {
            std::ofstream out(singleOutputFile);
            out << result.evidenceFull.dump(2);
        }

        std::cout << "\n======================================================\n"
                  << "  OCR Evidence Intelligence Engine v0.3\n"
                  << "======================================================\n\n";

        std::cout << "Document ID:     " << result.document.metadata.id << "\n";
        std::cout << "Pages Processed: " << result.document.metadata.pageCount << "\n";
        std::cout << "Source Items:    " << result.zeroLossReport.totalSourceItems << "\n\n";

        std::cout << "Confidence Breakdown:\n"
                  << "  High Confidence (>= " << config.routerConfig.highThreshold << "):   " << result.highConfidenceFields << "\n"
                  << "  Medium Confidence (>= " << config.routerConfig.mediumThreshold << "): " << result.mediumConfidenceFields << "\n"
                  << "  Low Confidence (< " << config.routerConfig.mediumThreshold << "):    " << result.lowConfidenceFields << "\n\n";

        std::cout << "Routing & Quality Breakdown:\n"
                  << "  Total Candidates:           " << result.routingStats.totalCandidates << "\n"
                  << "  High Confidence Filtered:   " << result.routingStats.highConfidence << "\n"
                  << "  Deterministically Resolved: " << result.routingStats.deterministicallyResolved << "\n"
                  << "  Structural Items Filtered:  " << result.routingStats.structuralItems << "\n"
                  << "  Placeholders Filtered:      " << result.routingStats.placeholders << "\n"
                  << "  LLM Eligible Fields:        " << result.routingStats.llmEligible << "\n"
                  << "  LLM Tasks Created:          " << (result.routingStats.llmTasksCreated > 0 ? result.routingStats.llmTasksCreated : result.routingStats.llmRequired) << "\n"
                  << "  Duplicate Tasks Removed:    " << result.routingStats.duplicateTasksRemoved << "\n\n";

        std::cout << "OCR Corrections:\n"
                  << "  Attempted: " << result.correctionsAttempted << "\n"
                  << "  Applied:   " << result.correctionsApplied << "\n"
                  << "  Unchanged: " << (result.correctionsAttempted - result.correctionsApplied) << "\n\n";

        std::cout << "LLM Routing:\n"
                  << "  LLM Required:     " << (result.llmRequired ? "YES" : "NO") << "\n"
                  << "  Unresolved Tasks: " << result.llmTaskCount << "\n"
                  << "  Estimated Tokens: " << ocr::evidence::TargetedLlmContextBuilder::estimateTokens(result.evidenceLlm) << "\n\n";

        std::cout << "Zero-Loss Quality Gate:\n"
                  << "  Source Items:      " << result.zeroLossReport.totalSourceItems << "\n"
                  << "  Represented Items: " << result.zeroLossReport.representedItems << "\n"
                  << "  Unaccounted Loss:  " << result.zeroLossReport.unaccountedLoss << " (Zero-Loss Validated)\n\n";

        // Reverse Path & Finalization:
        // Every document MUST produce Enhanced EDR and Final Document (schema: edmo-final-document-0.1)
        auto finalizationStart = std::chrono::steady_clock::now();

        if (result.llmRequired && llmPatchPath.empty()) {
            std::cout << "------------------------------------------------------\n"
                      << "  LLM Resolution Required: Pipeline Paused at Boundary\n"
                      << "------------------------------------------------------\n"
                      << "  Unresolved Tasks: " << result.llmTaskCount << "\n"
                      << "  Resolution Request written to: " << (std::filesystem::absolute(outputDir) / "llm-resolution-request.json").string() << "\n\n";

            nlohmann::json finalizationReport = {
                {"schema_version", "ocr-finalization-report-0.1"},
                {"document_id", result.document.metadata.id},
                {"status", "LLM_RESOLUTION_REQUIRED"},
                {"document_readiness", "RESOLUTION_PENDING"},
                {"reason", "UNRESOLVED_FIELDS_REQUIRE_LLM"},
                {"resolution_tasks_count", result.llmTaskCount}
            };

            auto reportPath = std::filesystem::path(outputDir) / "finalization-report.json";
            std::ofstream reportFile(reportPath);
            if (reportFile.is_open()) {
                reportFile << finalizationReport.dump(2);
            }

            return 0;
        }

        ocr::evidence::EdrPatchEngine patchEngine(ocr::evidence::ConfidenceFusionEngine(config.fusionPolicy));
        ocr::evidence::EnhancedEdrResult enhancedResult;
        nlohmann::json patchReportJson;

        if (!llmPatchPath.empty()) {
            std::cout << "------------------------------------------------------\n"
                      << "  Executing Reverse Path: LLM Patch Validation & Merge\n"
                      << "------------------------------------------------------\n";

            std::ifstream patchStream(llmPatchPath);
            if (!patchStream.is_open()) {
                throw std::runtime_error("Could not open LLM patch file: " + llmPatchPath);
            }
            nlohmann::json patchJson = nlohmann::json::parse(patchStream);

            enhancedResult = patchEngine.applyPatchesJson(result.document, result.fields, patchJson);
            patchReportJson = enhancedResult.patchReport.toJson();

            std::cout << "Patch Validation Summary:\n"
                      << "  Patches Received: " << enhancedResult.patchReport.received << "\n"
                      << "  Patches Accepted: " << enhancedResult.patchReport.accepted << "\n"
                      << "  Patches Rejected: " << enhancedResult.patchReport.rejected << "\n\n";
        } else {
            // LLM Skipped / Not Required Path: Create Enhanced EDR directly from trusted/deterministic forward evidence
            ocr::evidence::LlmPatchDocument emptyPatchDoc;
            emptyPatchDoc.documentId = result.document.metadata.id;
            enhancedResult = patchEngine.applyPatches(result.document, result.fields, emptyPatchDoc);
            patchReportJson = {
                {"status", "NOT_APPLICABLE"},
                {"reason", "NO_LLM_REQUIRED"},
                {"document_id", result.document.metadata.id},
                {"patches", {
                    {"received", 0},
                    {"accepted", 0},
                    {"rejected", 0}
                }}
            };
        }

        auto finalDocJson = ocr::evidence::FinalDocumentProjector::project(enhancedResult, result.document);

        // Validate Final Document against schema and Enhanced EDR
        auto valRes = ocr::evidence::FinalDocumentValidator::validate(
            finalDocJson,
            enhancedResult.enhancedEdrJson,
            result.document.metadata.id
        );
        if (!valRes.valid) {
            std::string errList = "";
            for (const auto& err : valRes.errors) {
                errList += (errList.empty() ? "" : "; ") + err;
            }
            throw std::runtime_error("Final Document validation failed: " + errList);
        }

        auto finalizationEnd = std::chrono::steady_clock::now();
        int64_t finalizationMs = std::chrono::duration_cast<std::chrono::milliseconds>(finalizationEnd - finalizationStart).count();
        if (finalizationMs == 0) finalizationMs = 1;

        auto finalizationReport = ocr::evidence::FinalDocumentProjector::buildFinalizationReport(
            enhancedResult,
            finalDocJson,
            finalizationMs
        );

        ocr::evidence::EvidenceJsonSerializer::saveEnhancedOutputs(
            enhancedResult.enhancedEdrJson,
            finalDocJson,
            patchReportJson,
            outputDir,
            finalizationReport.toJson(),
            true
        );

        std::cout << "Resolutions Breakdown:\n"
                  << "  OCR Confident:          " << enhancedResult.ocrConfidentCount << "\n"
                  << "  Deterministic Resolved: " << enhancedResult.deterministicCount << "\n"
                  << "  Normalized:             " << enhancedResult.normalizedCount << "\n"
                  << "  LLM Resolved:           " << enhancedResult.llmResolvedCount << "\n"
                  << "  AI Unresolved:          " << enhancedResult.aiUnresolvedCount << "\n"
                  << "  Unresolved:             " << enhancedResult.unresolvedCount << "\n"
                  << "  Rejected:               " << enhancedResult.rejectedCount << "\n\n";

        std::cout << "Final Structured Output Written to: " << std::filesystem::absolute(outputDir).string() << "\n"
                  << "  - enhanced_edr.json        (Canonical Evidence Layer)\n"
                  << "  - final_document.json      (Semantic Projection Layer)\n"
                  << "  - patch-report.json        (Patch Audit Report)\n"
                  << "  - finalization-report.json (Finalization Quality Report)\n\n";

        if (benchmark) {
            std::cout << "Performance Benchmarking:\n";
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "  Evidence Parsing:     " << result.timer.getStageDurationMs("evidence_parsing") << " ms\n";
            std::cout << "  Field Extraction:     " << result.timer.getStageDurationMs("field_extraction") << " ms\n";
            std::cout << "  Candidate Validation: " << result.timer.getStageDurationMs("candidate_validation") << " ms\n";
            std::cout << "  Correction:           " << result.timer.getStageDurationMs("correction") << " ms\n";
            std::cout << "  Evidence Fusion:      " << result.timer.getStageDurationMs("fusion") << " ms\n";
            std::cout << "  Routing:              " << result.timer.getStageDurationMs("routing") << " ms\n";
            std::cout << "  LLM Context:          " << result.timer.getStageDurationMs("llm_context") << " ms\n";
            std::cout << "  Audit & Export:       " << (result.timer.getStageDurationMs("audit") + result.timer.getStageDurationMs("serialization")) << " ms\n";
            std::cout << "  Total Execution:      " << result.timer.getTotalDurationMs() << " ms (Target: < 50ms/document)\n\n";
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] Pipeline failed: " << e.what() << "\n\n";
        return 1;
    }
}

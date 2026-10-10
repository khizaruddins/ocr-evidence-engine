#include <gtest/gtest.h>
#include <edmo/evidence/patch/LlmPatchValidator.hpp>

using namespace edmo::evidence;

namespace {

std::vector<EvidenceField> createTestFields() {
    std::vector<EvidenceField> fields;

    EvidenceField f1;
    f1.fieldId = "field_course_title_1";
    f1.field = "course_title";
    f1.llmTaskId = "resolve_001";
    f1.category = "course_record";
    f1.original.value = "MATHEMAT1CS FOR C0MPUTING";
    f1.original.confidence = 0.70f;
    f1.original.source.page = 1;
    f1.final.value = "MATHEMAT1CS FOR C0MPUTING";
    f1.final.confidence = 0.70f;
    f1.final.llmRequired = true;
    f1.llmEligible = true;
    fields.push_back(f1);

    EvidenceField f2;
    f2.fieldId = "field_student_id_1";
    f2.field = "student_id";
    f2.llmTaskId = "resolve_002";
    f2.category = "identity";
    f2.original.value = "GU-2024-9182";
    f2.original.confidence = 0.95f;
    f2.original.source.page = 1;
    f2.final.value = "GU-2024-9182";
    f2.final.confidence = 0.95f;
    f2.final.llmRequired = false;
    f2.llmEligible = false;
    fields.push_back(f2);

    return fields;
}

} // namespace

TEST(PatchValidatorTests, ValidPatchAccepted) {
    auto fields = createTestFields();
    std::string canonicalDocId = "doc_georgetown_123";

    LlmPatchDocument doc;
    doc.documentId = canonicalDocId;
    doc.schemaVersion = "edmo-llm-patch-0.1";

    LlmPatch p;
    p.patchId = "patch_001";
    p.taskId = "resolve_001";
    p.field = "course_title";
    p.originalValue = "MATHEMAT1CS FOR C0MPUTING";
    p.correctedValue = "MATHEMATICS FOR COMPUTING";
    p.llmConfidence = 0.96f;
    p.source.page = 1;
    p.evidenceSupported = true;
    doc.patches.push_back(p);

    auto report = LlmPatchValidator::validatePatches(canonicalDocId, fields, doc);
    EXPECT_EQ(report.received, 1);
    EXPECT_EQ(report.accepted, 1);
    EXPECT_EQ(report.rejected, 0);
}

TEST(PatchValidatorTests, RejectFieldScopeViolation) {
    auto fields = createTestFields();
    std::string canonicalDocId = "doc_georgetown_123";

    LlmPatchDocument doc;
    doc.documentId = canonicalDocId;
    doc.schemaVersion = "edmo-llm-patch-0.1";

    LlmPatch p;
    p.patchId = "patch_001";
    p.taskId = "resolve_001"; // Task resolve_001 is for course_title
    p.field = "course_code";  // Field scope violation!
    p.originalValue = "MATHEMAT1CS FOR C0MPUTING";
    p.correctedValue = "MATH 101";
    p.llmConfidence = 0.96f;
    p.source.page = 1;
    doc.patches.push_back(p);

    auto report = LlmPatchValidator::validatePatches(canonicalDocId, fields, doc);
    EXPECT_EQ(report.accepted, 0);
    EXPECT_EQ(report.rejected, 1);
    EXPECT_EQ(report.rejectedPatches[0].reasonCode, "FIELD_SCOPE_VIOLATION");
}

TEST(PatchValidatorTests, RejectDocumentIdMismatch) {
    auto fields = createTestFields();
    std::string canonicalDocId = "doc_georgetown_123";

    LlmPatchDocument doc;
    doc.documentId = "doc_wrong_456";
    doc.schemaVersion = "edmo-llm-patch-0.1";

    LlmPatch p;
    p.patchId = "patch_001";
    p.taskId = "resolve_001";
    p.field = "course_title";
    p.originalValue = "MATHEMAT1CS FOR C0MPUTING";
    p.correctedValue = "MATHEMATICS FOR COMPUTING";
    p.llmConfidence = 0.96f;
    p.source.page = 1;
    doc.patches.push_back(p);

    auto report = LlmPatchValidator::validatePatches(canonicalDocId, fields, doc);
    EXPECT_EQ(report.accepted, 0);
    EXPECT_EQ(report.rejected, 1);
    EXPECT_EQ(report.rejectedPatches[0].reasonCode, "DOCUMENT_ID_MISMATCH");
}

TEST(PatchValidatorTests, RejectUnknownTask) {
    auto fields = createTestFields();
    std::string canonicalDocId = "doc_georgetown_123";

    LlmPatchDocument doc;
    doc.documentId = canonicalDocId;
    doc.schemaVersion = "edmo-llm-patch-0.1";

    LlmPatch p;
    p.patchId = "patch_001";
    p.taskId = "resolve_999"; // Unknown task
    p.field = "course_title";
    p.originalValue = "MATHEMAT1CS FOR C0MPUTING";
    p.correctedValue = "MATHEMATICS FOR COMPUTING";
    p.llmConfidence = 0.96f;
    doc.patches.push_back(p);

    auto report = LlmPatchValidator::validatePatches(canonicalDocId, fields, doc);
    EXPECT_EQ(report.accepted, 0);
    EXPECT_EQ(report.rejected, 1);
    EXPECT_EQ(report.rejectedPatches[0].reasonCode, "UNKNOWN_TASK");
}

TEST(PatchValidatorTests, RejectConflictingPatches) {
    auto fields = createTestFields();
    std::string canonicalDocId = "doc_georgetown_123";

    LlmPatchDocument doc;
    doc.documentId = canonicalDocId;
    doc.schemaVersion = "edmo-llm-patch-0.1";

    LlmPatch p1;
    p1.patchId = "patch_001";
    p1.taskId = "resolve_001";
    p1.field = "course_title";
    p1.originalValue = "MATHEMAT1CS FOR C0MPUTING";
    p1.correctedValue = "MATHEMATICS FOR COMPUTING";
    p1.llmConfidence = 0.96f;
    p1.source.page = 1;
    doc.patches.push_back(p1);

    LlmPatch p2;
    p2.patchId = "patch_002";
    p2.taskId = "resolve_001"; // Same task with conflicting value
    p2.field = "course_title";
    p2.originalValue = "MATHEMAT1CS FOR C0MPUTING";
    p2.correctedValue = "MATHEMATICAL COMPUTING";
    p2.llmConfidence = 0.92f;
    p2.source.page = 1;
    doc.patches.push_back(p2);

    auto report = LlmPatchValidator::validatePatches(canonicalDocId, fields, doc);
    EXPECT_EQ(report.accepted, 0);
    EXPECT_EQ(report.rejected, 2);
    EXPECT_EQ(report.rejectedPatches[0].reasonCode, "CONFLICTING_PATCHES");
    EXPECT_EQ(report.rejectedPatches[1].reasonCode, "CONFLICTING_PATCHES");
}

TEST(PatchValidatorTests, AcceptResolutionResponseSchema) {
    auto fields = createTestFields();
    std::string canonicalDocId = "doc_georgetown_123";

    nlohmann::json responseJson = {
        {"schema_version", "edmo-llm-resolution-response-0.1"},
        {"document_id", canonicalDocId},
        {"resolutions", nlohmann::json::array({
            {
                {"task_id", "resolve_001"},
                {"field", "course_title"},
                {"status", "RESOLVED"},
                {"value", "MATHEMATICS FOR COMPUTING"},
                {"confidence", 0.98},
                {"resolution_method", "LLM"},
                {"reason", "OCR correction based on context"},
                {"source", {{"page", 1}}}
            }
        })}
    };

    auto report = LlmPatchValidator::validateJson(canonicalDocId, fields, responseJson);
    EXPECT_EQ(report.received, 1);
    EXPECT_EQ(report.accepted, 1);
    EXPECT_EQ(report.rejected, 0);
}

TEST(PatchValidatorTests, RejectOverwriteOfDeterministicField) {
    auto fields = createTestFields();
    fields[0].resolutionStatus = ResolutionStatus::Deterministic; // Deterministically resolved

    std::string canonicalDocId = "doc_georgetown_123";

    LlmPatchDocument doc;
    doc.documentId = canonicalDocId;
    doc.schemaVersion = "edmo-llm-resolution-response-0.1";

    LlmPatch p;
    p.patchId = "resolve_001";
    p.taskId = "resolve_001";
    p.field = "course_title";
    p.correctedValue = "NEW VALUE";
    p.llmConfidence = 0.99f;
    p.source.page = 1;
    doc.patches.push_back(p);

    auto report = LlmPatchValidator::validatePatches(canonicalDocId, fields, doc);
    EXPECT_EQ(report.accepted, 0);
    EXPECT_EQ(report.rejected, 1);
    EXPECT_EQ(report.rejectedPatches[0].reasonCode, "UNAUTHORIZED_FIELD_OVERWRITE");
}

TEST(PatchValidatorTests, RejectResolutionWithUnresolvedOrRejectedStatus) {
    auto fields = createTestFields();
    std::string canonicalDocId = "doc_georgetown_123";

    LlmPatchDocument doc;
    doc.documentId = canonicalDocId;
    doc.schemaVersion = "edmo-llm-resolution-response-0.1";

    LlmPatch p;
    p.patchId = "resolve_001";
    p.taskId = "resolve_001";
    p.field = "course_title";
    p.resolutionStatus = "UNRESOLVED";
    p.correctedValue = "MATHEMAT1CS FOR C0MPUTING";
    p.llmConfidence = 0.50f;
    p.source.page = 1;
    doc.patches.push_back(p);

    auto report = LlmPatchValidator::validatePatches(canonicalDocId, fields, doc);
    EXPECT_EQ(report.accepted, 0);
    EXPECT_EQ(report.rejected, 1);
    EXPECT_EQ(report.rejectedPatches[0].reasonCode, "STATUS_UNRESOLVED");
}

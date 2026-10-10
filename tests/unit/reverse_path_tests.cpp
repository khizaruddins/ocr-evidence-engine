#include <gtest/gtest.h>
#include <edmo/evidence/patch/EdrPatchEngine.hpp>
#include <edmo/evidence/projection/FinalDocumentProjector.hpp>

using namespace edmo::evidence;

namespace {

EvidenceDocument createSampleDocument() {
    EvidenceDocument doc;
    doc.metadata.id = "doc_test_reverse_123";
    doc.metadata.type = "transcript";
    doc.identity["student_name"] = "Sophia Jones";
    doc.identity["student_id"] = "712398456";
    doc.institution["name"] = "Georgetown University";
    doc.academic["degree"] = "Bachelor of Science";
    doc.academic["major"] = "Computer Science";
    return doc;
}

std::vector<EvidenceField> createSampleFields() {
    std::vector<EvidenceField> fields;

    EvidenceField f1;
    f1.fieldId = "field_student_name_1";
    f1.field = "student_name";
    f1.category = "identity";
    f1.original.value = "Sophia Jones";
    f1.original.confidence = 0.95f;
    f1.final.value = "Sophia Jones";
    f1.final.confidence = 0.95f;
    f1.resolutionStatus = ResolutionStatus::OcrConfident;
    fields.push_back(f1);

    EvidenceField f2;
    f2.fieldId = "field_course_code_1";
    f2.field = "course_code";
    f2.category = "course_record";
    f2.term = "Fall 2024";
    f2.status = "COMPLETED";
    f2.original.value = "CS 101";
    f2.original.confidence = 0.95f;
    f2.final.value = "CS 101";
    f2.final.confidence = 0.95f;
    f2.resolutionStatus = ResolutionStatus::OcrConfident;
    fields.push_back(f2);

    EvidenceField f3;
    f3.fieldId = "field_course_title_1";
    f3.field = "course_title";
    f3.category = "course_record";
    f3.term = "Fall 2024";
    f3.status = "COMPLETED";
    f3.llmTaskId = "resolve_001";
    f3.original.value = "MATHEMAT1CS FOR C0MPUTING";
    f3.original.confidence = 0.70f;
    f3.original.source.page = 1;
    f3.final.value = "MATHEMAT1CS FOR C0MPUTING";
    f3.final.confidence = 0.70f;
    f3.final.llmRequired = true;
    f3.llmEligible = true;
    f3.resolutionStatus = ResolutionStatus::LlmRequired;
    fields.push_back(f3);

    EvidenceField f4;
    f4.fieldId = "field_credits_1";
    f4.field = "credits";
    f4.category = "course_record";
    f4.term = "Fall 2024";
    f4.status = "COMPLETED";
    f4.original.value = "3.00";
    f4.original.confidence = 0.95f;
    f4.final.value = "3.00";
    f4.final.confidence = 0.95f;
    f4.resolutionStatus = ResolutionStatus::OcrConfident;
    fields.push_back(f4);

    EvidenceField f5;
    f5.fieldId = "field_grade_1";
    f5.field = "grade";
    f5.category = "course_record";
    f5.term = "Fall 2024";
    f5.status = "COMPLETED";
    f5.original.value = "A";
    f5.original.confidence = 0.95f;
    f5.final.value = "A";
    f5.final.confidence = 0.95f;
    f5.resolutionStatus = ResolutionStatus::OcrConfident;
    fields.push_back(f5);

    return fields;
}

} // namespace

TEST(ReversePathTests, EnhancedEdrAndFinalDocumentGeneration) {
    auto doc = createSampleDocument();
    auto fields = createSampleFields();

    LlmPatchDocument patchDoc;
    patchDoc.schemaVersion = "edmo-llm-patch-0.1";
    patchDoc.documentId = doc.metadata.id;
    patchDoc.model.provider = "vllm";
    patchDoc.model.model = "qwen3-8b";

    LlmPatch p;
    p.patchId = "patch_001";
    p.taskId = "resolve_001";
    p.field = "course_title";
    p.originalValue = "MATHEMAT1CS FOR C0MPUTING";
    p.correctedValue = "MATHEMATICS FOR COMPUTING";
    p.llmConfidence = 0.96f;
    p.source.page = 1;
    p.reasonCode = "OCR_CHARACTER_SUBSTITUTION";
    p.evidenceSupported = true;
    patchDoc.patches.push_back(p);

    EdrPatchEngine patchEngine;
    auto enhancedResult = patchEngine.applyPatches(doc, fields, patchDoc);

    // Verify Enhanced EDR structure and schema
    EXPECT_EQ(enhancedResult.schemaVersion, "edr-enhanced-0.1");
    EXPECT_EQ(enhancedResult.documentId, doc.metadata.id);
    EXPECT_EQ(enhancedResult.llmResolvedCount, 1);
    EXPECT_EQ(enhancedResult.ocrConfidentCount, 4);
    EXPECT_TRUE(enhancedResult.zeroLossVerified);

    const auto& enhJson = enhancedResult.enhancedEdrJson;
    EXPECT_EQ(enhJson["schema_version"], "edr-enhanced-0.1");
    EXPECT_EQ(enhJson["document_id"], doc.metadata.id);
    EXPECT_EQ(enhJson["audit"]["unaccounted_loss"], 0);

    // Verify Final Document semantic projection
    auto finalDoc = FinalDocumentProjector::project(enhancedResult, doc);
    EXPECT_EQ(finalDoc["document_type"], "transcript");
    EXPECT_EQ(finalDoc["student"]["name"], "Sophia Jones");
    EXPECT_EQ(finalDoc["institution"]["name"], "Georgetown University");
    EXPECT_EQ(finalDoc["program"]["major"], "Computer Science");

    ASSERT_EQ(finalDoc["courses"].size(), 1);
    const auto& course = finalDoc["courses"][0];
    EXPECT_EQ(course["course_code"], "CS 101");
    EXPECT_EQ(course["course_title"], "MATHEMATICS FOR COMPUTING");
    EXPECT_EQ(course["grade"], "A");
    EXPECT_EQ(course["resolution_method"], "LLM");
    EXPECT_NEAR(course["confidence"].get<float>(), 0.916f, 0.01f);

    // Verify that Final Document can be reconstructed at any time from Enhanced EDR JSON
    auto reconstructedDoc = FinalDocumentProjector::projectFromEnhancedJson(enhJson);
    EXPECT_EQ(reconstructedDoc["student"]["name"], "Sophia Jones");
    EXPECT_EQ(reconstructedDoc["institution"]["name"], "Georgetown University");
    ASSERT_EQ(reconstructedDoc["courses"].size(), 1);
    EXPECT_EQ(reconstructedDoc["courses"][0]["course_title"], "MATHEMATICS FOR COMPUTING");
}

#include <gtest/gtest.h>
#include <ocr/evidence/projection/FinalDocumentProjector.hpp>
#include <ocr/evidence/patch/EdrPatchEngine.hpp>

using namespace ocr::evidence;

namespace {

EvidenceDocument createMockTranscriptDoc() {
    EvidenceDocument doc;
    doc.metadata.id = "doc_georgetown_789";
    doc.metadata.type = "transcript";
    doc.metadata.pageCount = 2;
    doc.identity["student_name"] = "Marcus Thorne";
    doc.identity["student_id"] = "GU-2024-9182";
    doc.institution["name"] = "Georgetown University";
    doc.academic["degree"] = "Bachelor of Science";
    doc.academic["major"] = "Computer Science";
    doc.unclassifiedContent.push_back("Dean's List Award: Fall 2024");
    return doc;
}

std::vector<EvidenceField> createMockTranscriptFields() {
    std::vector<EvidenceField> fields;

    // Institution field
    EvidenceField fInst;
    fInst.fieldId = "field_inst_1";
    fInst.field = "institution_name";
    fInst.category = "identity";
    fInst.isSemanticValid = true;
    fInst.original.value = "Georgetown University";
    fInst.final.value = "Georgetown University";
    fInst.final.confidence = 0.98f;
    fInst.resolutionStatus = ResolutionStatus::OcrConfident;
    fields.push_back(fInst);

    // Student identity fields
    EvidenceField fName;
    fName.fieldId = "field_name_1";
    fName.field = "student_name";
    fName.category = "identity";
    fName.isSemanticValid = true;
    fName.original.value = "Marcus Thorne";
    fName.final.value = "Marcus Thorne";
    fName.final.confidence = 0.97f;
    fName.resolutionStatus = ResolutionStatus::OcrConfident;
    fields.push_back(fName);

    EvidenceField fId;
    fId.fieldId = "field_id_1";
    fId.field = "student_id";
    fId.category = "identity";
    fId.isSemanticValid = true;
    fId.original.value = "GU-2024-9182";
    fId.final.value = "GU-2024-9182";
    fId.final.confidence = 0.96f;
    fId.resolutionStatus = ResolutionStatus::OcrConfident;
    fields.push_back(fId);

    // Program fields
    EvidenceField fMajor;
    fMajor.fieldId = "field_maj_1";
    fMajor.field = "major";
    fMajor.category = "academic";
    fMajor.isSemanticValid = true;
    fMajor.original.value = "Computer Science";
    fMajor.final.value = "Computer Science";
    fMajor.final.confidence = 0.95f;
    fMajor.resolutionStatus = ResolutionStatus::OcrConfident;
    fields.push_back(fMajor);

    // Course 1 fields (LLM Resolved)
    EvidenceField c1Code;
    c1Code.fieldId = "field_course_code_1";
    c1Code.field = "course_code";
    c1Code.category = "course_record";
    c1Code.term = "Fall 2024";
    c1Code.status = "COMPLETED";
    c1Code.isSemanticValid = true;
    c1Code.original.value = "CS 101";
    c1Code.final.value = "CS 101";
    c1Code.final.confidence = 0.95f;
    c1Code.resolutionStatus = ResolutionStatus::OcrConfident;
    fields.push_back(c1Code);

    EvidenceField c1Title;
    c1Title.fieldId = "field_course_title_1";
    c1Title.field = "course_title";
    c1Title.category = "course_record";
    c1Title.term = "Fall 2024";
    c1Title.status = "COMPLETED";
    c1Title.isSemanticValid = true;
    c1Title.original.value = "MATHEMAT1CS FOR COMPUTING";
    c1Title.final.value = "MATHEMATICS FOR COMPUTING";
    c1Title.final.confidence = 0.94f;
    c1Title.resolutionStatus = ResolutionStatus::Resolved;
    c1Title.resolutionMethod = "LLM";
    fields.push_back(c1Title);

    EvidenceField c1Cred;
    c1Cred.fieldId = "field_credits_1";
    c1Cred.field = "credits";
    c1Cred.category = "course_record";
    c1Cred.term = "Fall 2024";
    c1Cred.status = "COMPLETED";
    c1Cred.isSemanticValid = true;
    c1Cred.original.value = "3.00";
    c1Cred.final.value = "3.00";
    c1Cred.final.confidence = 0.98f;
    c1Cred.resolutionStatus = ResolutionStatus::OcrConfident;
    fields.push_back(c1Cred);

    EvidenceField c1Grade;
    c1Grade.fieldId = "field_grade_1";
    c1Grade.field = "grade";
    c1Grade.category = "course_record";
    c1Grade.term = "Fall 2024";
    c1Grade.status = "COMPLETED";
    c1Grade.isSemanticValid = true;
    c1Grade.original.value = "A";
    c1Grade.final.value = "A";
    c1Grade.final.confidence = 0.99f;
    c1Grade.resolutionStatus = ResolutionStatus::OcrConfident;
    fields.push_back(c1Grade);

    // Summary fields
    EvidenceField fGpa;
    fGpa.fieldId = "field_gpa_1";
    fGpa.field = "cumulative_gpa";
    fGpa.category = "summary";
    fGpa.isSemanticValid = true;
    fGpa.original.value = "3.85";
    fGpa.final.value = "3.85";
    fGpa.final.confidence = 0.97f;
    fGpa.resolutionStatus = ResolutionStatus::OcrConfident;
    fields.push_back(fGpa);

    return fields;
}

} // namespace

TEST(FinalProjectorTests, ProjectsExpectedSchemaAndValues) {
    auto doc = createMockTranscriptDoc();
    auto fields = createMockTranscriptFields();

    EdrPatchEngine patchEngine;
    LlmPatchDocument emptyPatch;
    emptyPatch.documentId = doc.metadata.id;
    auto enhancedRes = patchEngine.applyPatches(doc, fields, emptyPatch);

    auto finalDoc = FinalDocumentProjector::project(enhancedRes, doc);

    EXPECT_EQ(finalDoc["schema_version"], "ocr-final-document-0.1");
    EXPECT_EQ(finalDoc["document"]["document_id"], "doc_georgetown_789");
    EXPECT_EQ(finalDoc["document"]["document_type"], "transcript");
    EXPECT_EQ(finalDoc["document"]["page_count"], 2);

    EXPECT_EQ(finalDoc["institution"]["name"], "Georgetown University");
    EXPECT_EQ(finalDoc["student"]["name"], "Marcus Thorne");
    EXPECT_EQ(finalDoc["student"]["student_id"], "GU-2024-9182");
    EXPECT_EQ(finalDoc["program"]["major"], "Computer Science");
    EXPECT_EQ(finalDoc["summary"]["cumulative_gpa"], 3.85f);

    ASSERT_EQ(finalDoc["courses"].size(), 1);
    const auto& c = finalDoc["courses"][0];
    EXPECT_EQ(c["course_code"], "CS 101");
    EXPECT_EQ(c["course_title"], "MATHEMATICS FOR COMPUTING");
    EXPECT_EQ(c["credits"], 3.0f);
    EXPECT_EQ(c["grade"], "A");
    EXPECT_EQ(c["status"], "COMPLETED");
    EXPECT_EQ(c["resolution_method"], "LLM");

    // Check additional sections preservation
    ASSERT_TRUE(finalDoc.contains("additional_sections"));
    EXPECT_EQ(finalDoc["additional_sections"].size(), 1);
    EXPECT_EQ(finalDoc["additional_sections"][0]["type"], "unclassified_content");

    // Check weighted semantic fields confidence calculation
    ASSERT_TRUE(finalDoc.contains("confidence"));
    EXPECT_EQ(finalDoc["confidence"]["method"], "WEIGHTED_SEMANTIC_FIELDS");
    float overall = finalDoc["confidence"]["overall"].get<float>();
    EXPECT_GE(overall, 0.90f);
    EXPECT_LE(overall, 1.0f);
}

TEST(FinalProjectorTests, MissingFieldsRemainNullWithoutFabrication) {
    EvidenceDocument emptyDoc;
    emptyDoc.metadata.id = "doc_empty_123";
    emptyDoc.metadata.type = "transcript";

    std::vector<EvidenceField> emptyFields;

    EdrPatchEngine patchEngine;
    LlmPatchDocument emptyPatch;
    emptyPatch.documentId = emptyDoc.metadata.id;
    auto enhancedRes = patchEngine.applyPatches(emptyDoc, emptyFields, emptyPatch);

    auto finalDoc = FinalDocumentProjector::project(enhancedRes, emptyDoc);

    EXPECT_EQ(finalDoc["schema_version"], "ocr-final-document-0.1");
    EXPECT_TRUE(finalDoc["institution"]["name"].is_null());
    EXPECT_TRUE(finalDoc["student"]["name"].is_null());
    EXPECT_TRUE(finalDoc["student"]["student_id"].is_null());
    EXPECT_TRUE(finalDoc["program"]["major"].is_null());
    EXPECT_TRUE(finalDoc["summary"]["cumulative_gpa"].is_null());
    EXPECT_EQ(finalDoc["courses"].size(), 0);
}

TEST(FinalProjectorTests, ReconstructFromEnhancedJsonDeterministically) {
    auto doc = createMockTranscriptDoc();
    auto fields = createMockTranscriptFields();

    EdrPatchEngine patchEngine;
    LlmPatchDocument emptyPatch;
    emptyPatch.documentId = doc.metadata.id;
    auto enhancedRes = patchEngine.applyPatches(doc, fields, emptyPatch);

    auto finalDoc1 = FinalDocumentProjector::project(enhancedRes, doc);
    auto finalDoc2 = FinalDocumentProjector::projectFromEnhancedJson(enhancedRes.enhancedEdrJson);

    EXPECT_EQ(finalDoc1["document"]["document_id"], finalDoc2["document"]["document_id"]);
    EXPECT_EQ(finalDoc1["student"]["name"], finalDoc2["student"]["name"]);
    EXPECT_EQ(finalDoc1["institution"]["name"], finalDoc2["institution"]["name"]);
    EXPECT_EQ(finalDoc1["courses"].size(), finalDoc2["courses"].size());
    EXPECT_EQ(finalDoc1["confidence"]["overall"], finalDoc2["confidence"]["overall"]);
}

TEST(FinalProjectorTests, FinalizationReportGeneratedCorrectly) {
    auto doc = createMockTranscriptDoc();
    auto fields = createMockTranscriptFields();

    EdrPatchEngine patchEngine;
    LlmPatchDocument emptyPatch;
    emptyPatch.documentId = doc.metadata.id;
    auto enhancedRes = patchEngine.applyPatches(doc, fields, emptyPatch);

    auto finalDoc = FinalDocumentProjector::project(enhancedRes, doc);
    auto report = FinalDocumentProjector::buildFinalizationReport(enhancedRes, finalDoc, 3);

    EXPECT_EQ(report.schemaVersion, "finalization-report-0.1");
    EXPECT_EQ(report.documentId, "doc_georgetown_789");
    EXPECT_EQ(report.status, "COMPLETED");
    EXPECT_GT(report.fieldsProjected, 0);
    EXPECT_EQ(report.executionTimeMs, 3);
}

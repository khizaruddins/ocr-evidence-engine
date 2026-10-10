#include <gtest/gtest.h>
#include <ocr/evidence/resolution/ResolutionAnalyzer.hpp>
#include <ocr/evidence/resolution/ResolutionStrategy.hpp>
#include <ocr/evidence/context/TargetedLlmContextBuilder.hpp>
#include <ocr/evidence/patch/EdrPatchEngine.hpp>
#include <ocr/evidence/projection/FinalDocumentProjector.hpp>
#include <ocr/evidence/pipeline/EvidencePipeline.hpp>
#include <nlohmann/json.hpp>

using namespace ocr::evidence;

namespace {

EvidenceDocument createBaseDoc() {
    EvidenceDocument doc;
    doc.metadata.id = "doc_test_123";
    doc.metadata.type = "transcript";
    doc.metadata.pageCount = 1;
    return doc;
}

} // namespace

// TEST 1: Major = Accounting & Finance. Strong label/value relationship.
TEST(ResolutionAnalyzerTests, Test1_MajorAccountingAndFinanceBypassesLlm) {
    ResolutionAnalyzer analyzer;
    auto doc = createBaseDoc();

    EvidenceField field;
    field.fieldId = "field_major_1";
    field.field = "major";
    field.category = "academic";
    field.sourceBlockId = "block_p1_8";
    field.labelBlockId = "block_p1_7";
    field.labelText = "BACHELORS MAJOR";
    field.semanticRelationship = "VALUE_FOR_LABEL";
    field.structuralClass = StructuralClass::StudentMetadata;
    field.original.value = "Accounting & Finance";
    field.original.confidence = 0.9129f;
    field.final.value = "Accounting & Finance";
    field.final.confidence = 0.9129f;

    std::vector<EvidenceField> allFields = {field};
    auto analysis = analyzer.analyzeField(field, doc, allFields);

    EXPECT_EQ(analysis.status, ResolutionStatus::Deterministic);
    EXPECT_EQ(analysis.strategy, ResolutionStrategy::None);
    EXPECT_TRUE(analysis.isTrusted);
    EXPECT_FALSE(analysis.llmRequired);
    EXPECT_TRUE(analysis.problem.empty());
}

// TEST 2: Student name has duplicated OCR.
TEST(ResolutionAnalyzerTests, Test2_StudentNameDuplicatedOcrTriggersLlmWithProblem) {
    ResolutionAnalyzer analyzer;
    auto doc = createBaseDoc();

    EvidenceField field;
    field.fieldId = "field_name_1";
    field.field = "student_name";
    field.category = "identity";
    field.sourceBlockId = "block_p1_4";
    field.labelBlockId = "block_p1_3";
    field.labelText = "NAME";
    field.structuralClass = StructuralClass::StudentMetadata;
    field.original.value = "Sophia Sophia Jones Jones";
    field.final.value = "Sophia Sophia Jones Jones";
    field.final.confidence = 0.95f;

    std::vector<EvidenceField> allFields = {field};
    auto analysis = analyzer.analyzeField(field, doc, allFields);

    EXPECT_EQ(analysis.status, ResolutionStatus::LlmRequired);
    EXPECT_EQ(analysis.strategy, ResolutionStrategy::TextLlm);
    EXPECT_FALSE(analysis.isTrusted);
    EXPECT_TRUE(analysis.llmRequired);
    EXPECT_EQ(analysis.problem.type, "AMBIGUOUS_FIELD_VALUE");
    EXPECT_FALSE(analysis.problem.description.empty());
    EXPECT_FALSE(analysis.problem.reason.empty());
    EXPECT_FALSE(analysis.problem.deterministicAttempt.empty());
    EXPECT_FALSE(analysis.problem.ambiguity.empty());
}

// TEST 3: Two conflicting majors.
TEST(ResolutionAnalyzerTests, Test3_ConflictingMajorsTriggersConflictingEvidenceProblem) {
    ResolutionAnalyzer analyzer;
    auto doc = createBaseDoc();

    EvidenceField f1;
    f1.fieldId = "field_major_1";
    f1.field = "major";
    f1.category = "academic";
    f1.original.value = "Accounting & Finance";
    f1.final.value = "Accounting & Finance";
    f1.final.confidence = 0.95f;
    f1.original.source.page = 1;

    EvidenceField f2;
    f2.fieldId = "field_major_2";
    f2.field = "major";
    f2.category = "academic";
    f2.original.value = "Computer Science";
    f2.final.value = "Computer Science";
    f2.final.confidence = 0.95f;
    f2.original.source.page = 1;

    std::vector<EvidenceField> allFields = {f1, f2};
    auto a1 = analyzer.analyzeField(f1, doc, allFields);
    auto a2 = analyzer.analyzeField(f2, doc, allFields);

    EXPECT_EQ(a1.status, ResolutionStatus::LlmRequired);
    EXPECT_EQ(a1.problem.type, "CONFLICTING_EVIDENCE");
    EXPECT_EQ(a2.status, ResolutionStatus::LlmRequired);
    EXPECT_EQ(a2.problem.type, "CONFLICTING_EVIDENCE");
}

// TEST 4: Field has no evidence -> AI_UNRESOLVED / INSUFFICIENT_EVIDENCE.
TEST(ResolutionAnalyzerTests, Test4_EmptyEvidenceResultsInAiUnresolved) {
    ResolutionAnalyzer analyzer;
    auto doc = createBaseDoc();

    EvidenceField field;
    field.fieldId = "field_inst_1";
    field.field = "institution_name";
    field.category = "institution";
    field.original.value = "";
    field.final.value = "";

    std::vector<EvidenceField> allFields = {field};
    auto analysis = analyzer.analyzeField(field, doc, allFields);

    EXPECT_EQ(analysis.status, ResolutionStatus::AiUnresolved);
    EXPECT_EQ(analysis.strategy, ResolutionStrategy::None);
    EXPECT_FALSE(analysis.llmRequired);
    EXPECT_EQ(analysis.problem.type, "INSUFFICIENT_EVIDENCE");
}

// TEST 5: Table column ambiguity (e.g. non-numeric credits).
TEST(ResolutionAnalyzerTests, Test5_AmbiguousTableColumnCredits) {
    ResolutionAnalyzer analyzer;
    auto doc = createBaseDoc();

    EvidenceField field;
    field.fieldId = "field_cred_1";
    field.field = "credits";
    field.category = "course_record";
    field.original.value = "3 A";
    field.final.value = "3 A";
    field.final.confidence = 0.85f;

    std::vector<EvidenceField> allFields = {field};
    auto analysis = analyzer.analyzeField(field, doc, allFields);

    EXPECT_EQ(analysis.status, ResolutionStatus::LlmRequired);
    EXPECT_EQ(analysis.problem.type, "TABLE_COLUMN_AMBIGUITY");
}

// TEST 6: LLM returns unknown task_id -> Rejected.
TEST(ResolutionAnalyzerTests, Test6_PatchEngineRejectsUnknownTaskId) {
    EdrPatchEngine patchEngine;
    auto doc = createBaseDoc();

    EvidenceField f;
    f.fieldId = "field_1";
    f.field = "student_name";
    f.llmTaskId = "resolve_001";
    f.resolutionStatus = ResolutionStatus::LlmRequired;

    LlmPatchDocument patchDoc;
    patchDoc.schemaVersion = "ocr-llm-resolution-response-0.1";
    patchDoc.documentId = doc.metadata.id;

    LlmPatch p;
    p.patchId = "patch_001";
    p.taskId = "resolve_999_unknown"; // Unknown task ID
    p.field = "student_name";
    p.correctedValue = "John Doe";
    p.llmConfidence = 0.98f;
    p.resolutionStatus = "RESOLVED";
    patchDoc.patches.push_back(p);

    auto result = patchEngine.applyPatches(doc, {f}, patchDoc);
    EXPECT_EQ(result.patchReport.accepted, 0);
    EXPECT_EQ(result.patchReport.rejected, 1);
    EXPECT_EQ(result.patchReport.rejectedPatches[0].reasonCode, "UNKNOWN_TASK");
}

// TEST 7: LLM attempts to modify deterministic field -> Rejected.
TEST(ResolutionAnalyzerTests, Test7_PatchEngineRejectsDeterministicFieldModification) {
    EdrPatchEngine patchEngine;
    auto doc = createBaseDoc();

    EvidenceField f;
    f.fieldId = "field_major_1";
    f.field = "major";
    f.llmTaskId = "resolve_001";
    f.resolutionStatus = ResolutionStatus::Deterministic; // Deterministic!

    LlmPatchDocument patchDoc;
    patchDoc.schemaVersion = "ocr-llm-resolution-response-0.1";
    patchDoc.documentId = doc.metadata.id;

    LlmPatch p;
    p.patchId = "patch_001";
    p.taskId = "resolve_001";
    p.field = "major";
    p.correctedValue = "Art History";
    p.llmConfidence = 0.98f;
    p.resolutionStatus = "RESOLVED";
    patchDoc.patches.push_back(p);

    auto result = patchEngine.applyPatches(doc, {f}, patchDoc);
    EXPECT_EQ(result.patchReport.accepted, 0);
    EXPECT_EQ(result.patchReport.rejected, 1);
    EXPECT_EQ(result.patchReport.rejectedPatches[0].reasonCode, "UNAUTHORIZED_FIELD_OVERWRITE");
}

// TEST 8: LLM returns unsupported value / invalid status -> Rejected.
TEST(ResolutionAnalyzerTests, Test8_PatchEngineRejectsUnsupportedResolutionStatus) {
    EdrPatchEngine patchEngine;
    auto doc = createBaseDoc();

    EvidenceField f;
    f.fieldId = "field_1";
    f.field = "student_name";
    f.llmTaskId = "resolve_001";
    f.resolutionStatus = ResolutionStatus::LlmRequired;

    LlmPatchDocument patchDoc;
    patchDoc.schemaVersion = "ocr-llm-resolution-response-0.1";
    patchDoc.documentId = doc.metadata.id;

    LlmPatch p;
    p.patchId = "patch_001";
    p.taskId = "resolve_001";
    p.field = "student_name";
    p.correctedValue = "Jane Doe";
    p.llmConfidence = 0.98f;
    p.resolutionStatus = "INVALID_STATUS_VALUE"; // Invalid status
    patchDoc.patches.push_back(p);

    auto result = patchEngine.applyPatches(doc, {f}, patchDoc);
    EXPECT_EQ(result.patchReport.accepted, 0);
    EXPECT_EQ(result.patchReport.rejected, 1);
    EXPECT_EQ(result.patchReport.rejectedPatches[0].reasonCode, "INVALID_STATUS");
}

// TEST 9: LLM resolves all mandatory tasks -> Merged and projected to final document.
TEST(ResolutionAnalyzerTests, Test9_LlmResolvesAllMandatoryTasksSuccessfully) {
    EdrPatchEngine patchEngine;
    auto doc = createBaseDoc();
    doc.identity["student_id"] = "12345";

    EvidenceField f;
    f.fieldId = "field_name_1";
    f.field = "student_name";
    f.category = "identity";
    f.llmTaskId = "resolve_001";
    f.original.value = "Sophia Soph1a Jones";
    f.final.value = "Sophia Soph1a Jones";
    f.resolutionStatus = ResolutionStatus::LlmRequired;

    LlmPatchDocument patchDoc;
    patchDoc.schemaVersion = "ocr-llm-resolution-response-0.1";
    patchDoc.documentId = doc.metadata.id;

    LlmPatch p;
    p.patchId = "patch_001";
    p.taskId = "resolve_001";
    p.field = "student_name";
    p.originalValue = "Sophia Soph1a Jones";
    p.correctedValue = "Sophia Jones";
    p.llmConfidence = 0.99f;
    p.resolutionStatus = "RESOLVED";
    patchDoc.patches.push_back(p);

    auto result = patchEngine.applyPatches(doc, {f}, patchDoc);
    EXPECT_EQ(result.patchReport.accepted, 1);
    EXPECT_EQ(result.llmResolvedCount, 1);
    EXPECT_EQ(result.unresolvedCount, 0);
    EXPECT_EQ(result.fields[0].final.value, "Sophia Jones");
    EXPECT_EQ(result.fields[0].resolutionStatus, ResolutionStatus::Resolved);

    auto finalDoc = FinalDocumentProjector::project(result, doc);
    EXPECT_EQ(finalDoc["document_readiness"], "READY");
    EXPECT_EQ(finalDoc["student"]["name"], "Sophia Jones");
    EXPECT_EQ(finalDoc["resolution"]["llm_resolved"], 1);
}

// TEST 10: Targeted vs Expanded context generation.
TEST(ResolutionAnalyzerTests, Test10_TargetedAndExpandedContextsDifferMeaningfully) {
    auto doc = createBaseDoc();
    EvidencePage p1;
    p1.pageNumber = 1;
    EvidenceBlock b1;
    b1.id = "blk_1";
    b1.text = "Introductory text";
    p1.blocks.push_back(b1);
    doc.pages.push_back(p1);

    EvidenceField f;
    f.fieldId = "field_name_1";
    f.field = "student_name";
    f.category = "identity";
    f.sourceBlockId = "blk_1";
    f.final.llmRequired = true;
    f.original.value = "Ambiguous Name";

    auto targetedReq = TargetedLlmContextBuilder::buildResolutionRequest(doc, {f}, ContextLevel::Targeted);
    auto expandedReq = TargetedLlmContextBuilder::buildResolutionRequest(doc, {f}, ContextLevel::Expanded);

    EXPECT_EQ(targetedReq["context_level"], "TARGETED");
    EXPECT_EQ(expandedReq["context_level"], "EXPANDED");
    EXPECT_GE(expandedReq["tasks"][0]["evidence"]["context"].size(),
              targetedReq["tasks"][0]["evidence"]["context"].size());
}

// TEST 11: AI exhausted all strategies -> AI_UNRESOLVED.
TEST(ResolutionAnalyzerTests, Test11_AiExhaustionSetsAiUnresolved) {
    EvidenceField f;
    f.fieldId = "field_name_1";
    f.field = "student_name";
    f.category = "identity";
    f.resolutionStatus = ResolutionStatus::AiUnresolved;
    f.resolutionStrategy = ResolutionStrategy::None;
    f.aiWorkflowState = AiWorkflowState::Failed;
    f.aiAttemptCount = 2;

    auto json = f.toJson();
    EXPECT_EQ(json["resolution_status"], "AI_UNRESOLVED");
    EXPECT_EQ(json["ai_workflow_state"], "FAILED");
    EXPECT_EQ(json["ai_attempt_count"], 2);
}

// TEST 12: Clean document with zero AI tasks.
TEST(ResolutionAnalyzerTests, Test12_CleanDocumentBypassesAiDirectToFinal) {
    ResolutionAnalyzer analyzer;
    auto doc = createBaseDoc();

    EvidenceField f1;
    f1.fieldId = "f1";
    f1.field = "student_name";
    f1.category = "identity";
    f1.labelText = "NAME";
    f1.original.value = "John Doe";
    f1.final.value = "John Doe";
    f1.final.confidence = 0.98f;

    EvidenceField f2;
    f2.fieldId = "f2";
    f2.field = "major";
    f2.category = "academic";
    f2.labelText = "MAJOR";
    f2.original.value = "Economics";
    f2.final.value = "Economics";
    f2.final.confidence = 0.98f;

    std::vector<EvidenceField> fields = {f1, f2};
    auto summary = analyzer.analyzeDocument(fields, doc);

    EXPECT_TRUE(summary.fastPathEligible);
    EXPECT_EQ(summary.llmRequiredCount, 0);
    EXPECT_FALSE(summary.hasMandatoryUnresolved);
}

// TEST 13: Table Column Association & Row Companions Resolve Course Title Deterministically
TEST(ResolutionAnalyzerTests, Test13_TableColumnAssociationResolvesCourseTitleDeterministically) {
    RouterConfig config;
    config.highThreshold = 0.95f; // Strict high threshold
    ResolutionAnalyzer analyzer(config);
    auto doc = createBaseDoc();

    EvidenceTable tbl;
    tbl.id = "table_p1_1";
    tbl.pageStart = 1;
    tbl.pageEnd = 1;
    tbl.columns.push_back({"course_code", "course_code", 0, 1.0f, {}});
    tbl.columns.push_back({"course_title", "course_title", 1, 1.0f, {}});
    tbl.columns.push_back({"credits", "credits", 2, 1.0f, {}});
    tbl.columns.push_back({"grade", "grade", 3, 1.0f, {}});
    doc.tables.push_back(tbl);

    EvidenceField codeField;
    codeField.fieldId = "field_course_code_11";
    codeField.field = "course_code";
    codeField.category = "course_record";
    codeField.structuralClass = StructuralClass::CourseRow;
    codeField.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
    codeField.original.value = "BUSI 499";
    codeField.final.value = "BUSI 499";
    codeField.original.confidence = 0.92f;
    codeField.final.confidence = 0.92f;

    EvidenceField titleField;
    titleField.fieldId = "field_course_title_11";
    titleField.field = "course_title";
    titleField.category = "course_record";
    titleField.structuralClass = StructuralClass::CourseRow;
    titleField.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
    titleField.sourceBlockId = "block_src_course_title";
    titleField.labelBlockId = "table_p1_1";
    titleField.original.value = "Personal and Professional Development";
    titleField.final.value = "Personal and Professional Development";
    titleField.original.confidence = 0.9207f; // Moderate confidence below 0.95
    titleField.final.confidence = 0.9207f;
    titleField.nearbyEvidence = {"Code: BUSI 499", "Credits: 3", "Grade: B"};

    EvidenceField credField;
    credField.fieldId = "field_credits_11";
    credField.field = "credits";
    credField.category = "course_record";
    credField.structuralClass = StructuralClass::CourseRow;
    credField.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
    credField.original.value = "3";
    credField.final.value = "3";
    credField.original.confidence = 0.92f;
    credField.final.confidence = 0.92f;

    EvidenceField gradeField;
    gradeField.fieldId = "field_grade_11";
    gradeField.field = "grade";
    gradeField.category = "course_record";
    gradeField.structuralClass = StructuralClass::CourseRow;
    gradeField.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
    gradeField.original.value = "B";
    gradeField.final.value = "B";
    gradeField.original.confidence = 0.92f;
    gradeField.final.confidence = 0.92f;

    std::vector<EvidenceField> allFields = {codeField, titleField, credField, gradeField};
    auto analysis = analyzer.analyzeField(titleField, doc, allFields);

    EXPECT_EQ(analysis.status, ResolutionStatus::Deterministic);
    EXPECT_EQ(analysis.strategy, ResolutionStrategy::None);
    EXPECT_TRUE(analysis.isTrusted);
    EXPECT_FALSE(analysis.llmRequired);
    EXPECT_EQ(analysis.reasonCode, "DETERMINISTIC_TABLE_RECORD");
    EXPECT_TRUE(analysis.problem.empty());
}

// TEST 14: Multiple Course Rows Resolved Deterministically With Relationships
TEST(ResolutionAnalyzerTests, Test14_MultipleCourseRowsResolvedDeterministicallyWithRelationships) {
    RouterConfig config;
    config.highThreshold = 0.95f;
    ResolutionAnalyzer analyzer(config);
    auto doc = createBaseDoc();

    EvidenceTable tbl;
    tbl.id = "table_p1_1";
    tbl.pageStart = 1;
    tbl.pageEnd = 1;
    doc.tables.push_back(tbl);

    struct RowData {
        std::string code;
        std::string title;
        std::string credits;
        std::string grade;
        float conf;
    };

    std::vector<RowData> rows = {
        {"ACCT 101", "Intro to Accounting", "3", "A", 0.91f},
        {"BUSI 101", "Foundations of Business", "3", "A", 0.93f},
        {"FINC 301", "Corporate Finance", "3", "B", 0.92f},
        {"ACCT 301", "Business Accounting Systems", "3", "B", 0.94f}
    };

    std::vector<EvidenceField> allFields;
    for (size_t i = 0; i < rows.size(); ++i) {
        int idx = static_cast<int>(i + 1);
        EvidenceField cf, tf, crf, gf;

        cf.fieldId = "field_course_code_" + std::to_string(idx);
        cf.field = "course_code";
        cf.category = "course_record";
        cf.structuralClass = StructuralClass::CourseRow;
        cf.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
        cf.final.value = rows[i].code;
        cf.final.confidence = rows[i].conf;

        tf.fieldId = "field_course_title_" + std::to_string(idx);
        tf.field = "course_title";
        tf.category = "course_record";
        tf.structuralClass = StructuralClass::CourseRow;
        tf.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
        tf.final.value = rows[i].title;
        tf.final.confidence = rows[i].conf;
        tf.nearbyEvidence = {"Code: " + rows[i].code, "Credits: " + rows[i].credits, "Grade: " + rows[i].grade};

        crf.fieldId = "field_credits_" + std::to_string(idx);
        crf.field = "credits";
        crf.category = "course_record";
        crf.structuralClass = StructuralClass::CourseRow;
        crf.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
        crf.final.value = rows[i].credits;
        crf.final.confidence = rows[i].conf;

        gf.fieldId = "field_grade_" + std::to_string(idx);
        gf.field = "grade";
        gf.category = "course_record";
        gf.structuralClass = StructuralClass::CourseRow;
        gf.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
        gf.final.value = rows[i].grade;
        gf.final.confidence = rows[i].conf;

        allFields.push_back(cf);
        allFields.push_back(tf);
        allFields.push_back(crf);
        allFields.push_back(gf);
    }

    auto summary = analyzer.analyzeDocument(allFields, doc);
    EXPECT_TRUE(summary.fastPathEligible);
    EXPECT_EQ(summary.llmRequiredCount, 0);
    EXPECT_EQ(summary.deterministicCount, static_cast<int>(allFields.size()));
}

// TEST 15: Genuinely Low-Confidence / No-Structure Course Title Becomes Ambiguous
TEST(ResolutionAnalyzerTests, Test15_LowConfidenceOrNoStructureCourseTitleBecomesAmbiguous) {
    RouterConfig config;
    config.highThreshold = 0.95f;
    ResolutionAnalyzer analyzer(config);
    auto doc = createBaseDoc(); // No tables in doc

    EvidenceField corruptedField;
    corruptedField.fieldId = "field_course_title_1";
    corruptedField.field = "course_title";
    corruptedField.category = "course_record";
    corruptedField.structuralClass = StructuralClass::CourseRow;
    corruptedField.original.value = "Unreadable %%&& Noise";
    corruptedField.final.value = "Unreadable %%&& Noise";
    corruptedField.original.confidence = 0.35f; // Genuinely low confidence
    corruptedField.final.confidence = 0.35f;

    std::vector<EvidenceField> allFields = {corruptedField};
    auto analysis = analyzer.analyzeField(corruptedField, doc, allFields);

    EXPECT_EQ(analysis.status, ResolutionStatus::LlmRequired);
    EXPECT_FALSE(analysis.isTrusted);
    EXPECT_TRUE(analysis.llmRequired);
    EXPECT_TRUE(analysis.isAmbiguous);
}

// TEST 16: Conflicting Course Title Candidates Requires LLM
TEST(ResolutionAnalyzerTests, Test16_ConflictingCourseTitleCandidatesRequiresLlm) {
    RouterConfig config;
    ResolutionAnalyzer analyzer(config);
    auto doc = createBaseDoc();

    EvidenceField conflictingField;
    conflictingField.fieldId = "field_course_title_1";
    conflictingField.field = "course_title";
    conflictingField.category = "course_record";
    conflictingField.structuralClass = StructuralClass::CourseRow;
    conflictingField.final.value = "Accounting Systems I";
    conflictingField.final.confidence = 0.90f;
    conflictingField.conflictingCandidates = {"Accounting Systems I", "Advanced Accounting Practice"};

    std::vector<EvidenceField> allFields = {conflictingField};
    auto analysis = analyzer.analyzeField(conflictingField, doc, allFields);

    EXPECT_EQ(analysis.status, ResolutionStatus::LlmRequired);
    EXPECT_TRUE(analysis.llmRequired);
    EXPECT_EQ(analysis.problem.type, "CONFLICTING_EVIDENCE");
}

// =========================================================================
// Fix #5 Regression Tests
// =========================================================================

// Fix #5 - TEST 1: BUSI 499 credits = 3, grade = B resolved deterministically without LLM
TEST(ResolutionAnalyzerTests, Test17_Fix5_Test1_Busi499CreditsResolvedDeterministicallyWithoutLlm) {
    RouterConfig config;
    ResolutionAnalyzer analyzer(config);
    auto doc = createBaseDoc();

    EvidenceTable tbl;
    tbl.id = "table_p1_1";
    tbl.pageStart = 1;
    tbl.pageEnd = 1;
    tbl.columns = {
        {"course_code", "course_code", 0, 1.0f, {}},
        {"course_title", "course_title", 1, 1.0f, {}},
        {"credits", "credits", 2, 1.0f, {}},
        {"grade", "grade", 3, 1.0f, {}}
    };
    doc.tables.push_back(tbl);

    EvidenceField codeField;
    codeField.fieldId = "field_course_code_1";
    codeField.field = "course_code";
    codeField.category = "course_record";
    codeField.structuralClass = StructuralClass::CourseRow;
    codeField.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
    codeField.sourceBlockId = "block_src_course_code";
    codeField.labelBlockId = "table_p1_1";
    codeField.final.value = "BUSI 499";
    codeField.final.confidence = 0.92f;
    codeField.original.source.page = 1;
    codeField.original.source.bbox = {325, 2435, 300, 35};

    EvidenceField titleField;
    titleField.fieldId = "field_course_title_1";
    titleField.field = "course_title";
    titleField.category = "course_record";
    titleField.structuralClass = StructuralClass::CourseRow;
    titleField.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
    titleField.sourceBlockId = "block_src_course_title";
    titleField.labelBlockId = "table_p1_1";
    titleField.final.value = "Personal and Professional Development";
    titleField.final.confidence = 0.93f;
    titleField.original.source.page = 1;
    titleField.original.source.bbox = {757, 2438, 745, 35};
    titleField.nearbyEvidence = {"Code: BUSI 499", "Credits: 3", "Status: REGISTERED"};

    // Note: raw OCR confidence 0.84608f is below 0.85f mediumThreshold
    EvidenceField creditsField;
    creditsField.fieldId = "field_credits_1";
    creditsField.field = "credits";
    creditsField.category = "course_record";
    creditsField.structuralClass = StructuralClass::CourseRow;
    creditsField.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
    creditsField.sourceBlockId = "block_src_credits";
    creditsField.labelBlockId = "table_p1_1";
    creditsField.original.value = "3";
    creditsField.original.confidence = 0.84608f;
    creditsField.final.value = "3";
    creditsField.final.confidence = 0.84608f;
    creditsField.original.source.page = 1;
    creditsField.original.source.bbox = {1569, 2439, 19, 28};
    creditsField.nearbyEvidence = {"Code: BUSI 499", "Credits: 3", "Status: REGISTERED"};

    EvidenceField gradeField;
    gradeField.fieldId = "field_grade_1";
    gradeField.field = "grade";
    gradeField.category = "course_record";
    gradeField.structuralClass = StructuralClass::CourseRow;
    gradeField.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
    gradeField.sourceBlockId = "block_src_grade";
    gradeField.labelBlockId = "table_p1_1";
    gradeField.final.value = "B";
    gradeField.final.confidence = 0.92f;
    gradeField.original.source.page = 1;
    gradeField.original.source.bbox = {1847, 2439, 22, 27};

    std::vector<EvidenceField> allFields = {codeField, titleField, creditsField, gradeField};

    auto analysis = analyzer.analyzeField(creditsField, doc, allFields);
    EXPECT_EQ(analysis.status, ResolutionStatus::Deterministic);
    EXPECT_FALSE(analysis.llmRequired);
    EXPECT_TRUE(analysis.isTrusted);
    EXPECT_EQ(analysis.reasonCode, "DETERMINISTIC_CREDITS_RESOLUTION");
    EXPECT_EQ(creditsField.final.value, "3");

    auto gradeAnalysis = analyzer.analyzeField(gradeField, doc, allFields);
    EXPECT_EQ(gradeAnalysis.status, ResolutionStatus::Deterministic);
    EXPECT_FALSE(gradeAnalysis.llmRequired);
    EXPECT_EQ(gradeField.final.value, "B");
}

// Fix #5 - TEST 2: At least 3 additional transcript rows with credits preserved correctly
TEST(ResolutionAnalyzerTests, Test18_Fix5_Test2_MultipleTranscriptRowsCreditsPreservedDeterministically) {
    RouterConfig config;
    ResolutionAnalyzer analyzer(config);
    auto doc = createBaseDoc();

    EvidenceTable tbl;
    tbl.id = "table_p1_1";
    tbl.pageStart = 1;
    tbl.pageEnd = 1;
    tbl.columns = {
        {"course_code", "course_code", 0, 1.0f, {}},
        {"course_title", "course_title", 1, 1.0f, {}},
        {"credits", "credits", 2, 1.0f, {}},
        {"grade", "grade", 3, 1.0f, {}}
    };
    doc.tables.push_back(tbl);

    struct CourseRowInfo {
        std::string code;
        std::string title;
        std::string credits;
        std::string grade;
        float creditsConf;
    };

    std::vector<CourseRowInfo> testCourses = {
        {"ACCT 101", "Intro to Accounting", "3", "A", 0.82f},
        {"ACCT 102", "Introduction to Accounting II", "3.0", "A", 0.84f},
        {"FINC 201", "Corporate Finance", "4", "A", 0.81f},
        {"MIS 301", "Management Information Systems", "3", "A", 0.83f}
    };

    std::vector<EvidenceField> allFields;
    for (size_t i = 0; i < testCourses.size(); ++i) {
        int idx = static_cast<int>(i + 1);
        int yCoord = 500 + idx * 40;

        EvidenceField cf;
        cf.fieldId = "field_course_code_" + std::to_string(idx);
        cf.field = "course_code";
        cf.category = "course_record";
        cf.structuralClass = StructuralClass::CourseRow;
        cf.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
        cf.labelBlockId = "table_p1_1";
        cf.final.value = testCourses[i].code;
        cf.final.confidence = 0.95f;
        cf.original.source.page = 1;
        cf.original.source.bbox = {300, yCoord, 200, 30};

        EvidenceField tf;
        tf.fieldId = "field_course_title_" + std::to_string(idx);
        tf.field = "course_title";
        tf.category = "course_record";
        tf.structuralClass = StructuralClass::CourseRow;
        tf.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
        tf.labelBlockId = "table_p1_1";
        tf.final.value = testCourses[i].title;
        tf.final.confidence = 0.95f;
        tf.original.source.page = 1;
        tf.original.source.bbox = {550, yCoord, 500, 30};
        tf.nearbyEvidence = {"Code: " + testCourses[i].code, "Credits: " + testCourses[i].credits};

        EvidenceField crf;
        crf.fieldId = "field_credits_" + std::to_string(idx);
        crf.field = "credits";
        crf.category = "course_record";
        crf.structuralClass = StructuralClass::CourseRow;
        crf.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
        crf.sourceBlockId = "block_src_credits";
        crf.labelBlockId = "table_p1_1";
        crf.original.value = testCourses[i].credits;
        crf.original.confidence = testCourses[i].creditsConf;
        crf.final.value = testCourses[i].credits;
        crf.final.confidence = testCourses[i].creditsConf;
        crf.original.source.page = 1;
        crf.original.source.bbox = {1100, yCoord, 30, 25};
        crf.nearbyEvidence = {"Code: " + testCourses[i].code, "Credits: " + testCourses[i].credits};

        EvidenceField gf;
        gf.fieldId = "field_grade_" + std::to_string(idx);
        gf.field = "grade";
        gf.category = "course_record";
        gf.structuralClass = StructuralClass::CourseRow;
        gf.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
        gf.labelBlockId = "table_p1_1";
        gf.final.value = testCourses[i].grade;
        gf.final.confidence = 0.95f;
        gf.original.source.page = 1;
        gf.original.source.bbox = {1200, yCoord, 30, 25};

        allFields.push_back(cf);
        allFields.push_back(tf);
        allFields.push_back(crf);
        allFields.push_back(gf);
    }

    // Verify all credit fields resolve deterministically without LLM
    for (size_t i = 0; i < testCourses.size(); ++i) {
        int idx = static_cast<int>(i + 1);
        std::string targetId = "field_credits_" + std::to_string(idx);
        auto it = std::find_if(allFields.begin(), allFields.end(), [&](const EvidenceField& f) {
            return f.fieldId == targetId;
        });
        ASSERT_NE(it, allFields.end());
        auto analysis = analyzer.analyzeField(*it, doc, allFields);
        EXPECT_EQ(analysis.status, ResolutionStatus::Deterministic);
        EXPECT_FALSE(analysis.llmRequired);
        EXPECT_EQ(analysis.reasonCode, "DETERMINISTIC_CREDITS_RESOLUTION");
        EXPECT_EQ(it->final.value, testCourses[i].credits);
    }
}

// Fix #5 - TEST 3: Numeric value outside credits column must NOT be classified as credits
TEST(ResolutionAnalyzerTests, Test19_Fix5_Test3_NumericValueOutsideCreditsColumnNotClassifiedAsCredits) {
    RouterConfig config;
    ResolutionAnalyzer analyzer(config);
    auto doc = createBaseDoc();

    // Field 1: Year "2024" in an academic date field
    EvidenceField yearField;
    yearField.fieldId = "field_year_1";
    yearField.field = "term_year";
    yearField.category = "academic";
    yearField.final.value = "2024";
    yearField.final.confidence = 0.85f;

    std::vector<EvidenceField> allFields = {yearField};
    float structScore = analyzer.calculateStructuralCreditsConfidence(yearField, doc, allFields);
    EXPECT_LT(structScore, 0.50f); // Must NOT pass as credits

    // Field 2: Section number "01" outside credits column, in course_record
    EvidenceField sectionField;
    sectionField.fieldId = "field_section_1";
    sectionField.field = "course_section";
    sectionField.category = "course_record";
    sectionField.sourceBlockId = "block_src_section";
    sectionField.final.value = "01";
    sectionField.final.confidence = 0.85f;

    allFields.push_back(sectionField);
    float sectionScore = analyzer.calculateStructuralCreditsConfidence(sectionField, doc, allFields);
    EXPECT_LT(sectionScore, 0.50f); // Must NOT pass as credits

    // Field 3: A field claimed as "credits" but with source from course_code column and impossible credit value "499"
    EvidenceField bogusCredits;
    bogusCredits.fieldId = "field_credits_bogus";
    bogusCredits.field = "credits";
    bogusCredits.category = "course_record";
    bogusCredits.sourceBlockId = "block_src_course_code";
    bogusCredits.final.value = "499";
    bogusCredits.final.confidence = 0.80f;

    allFields.push_back(bogusCredits);
    float bogusScore = analyzer.calculateStructuralCreditsConfidence(bogusCredits, doc, allFields);
    EXPECT_EQ(bogusScore, 0.0f); // Impossible credit value -> 0.0f
}

// Fix #5 - TEST 4: Two numeric candidates in different rows remain associated with their correct rows
TEST(ResolutionAnalyzerTests, Test20_Fix5_Test4_TwoNumericCandidatesInDifferentRowsRemainAssociatedWithCorrectRows) {
    RouterConfig config;
    ResolutionAnalyzer analyzer(config);
    auto doc = createBaseDoc();

    EvidenceTable tbl;
    tbl.id = "table_p1_1";
    tbl.pageStart = 1;
    tbl.pageEnd = 1;
    tbl.columns = {
        {"course_code", "course_code", 0, 1.0f, {}},
        {"credits", "credits", 1, 1.0f, {}},
        {"grade", "grade", 2, 1.0f, {}}
    };
    doc.tables.push_back(tbl);

    // Row 1: BUSI 499, credits = 3, grade = B
    EvidenceField c1;
    c1.fieldId = "field_course_code_1";
    c1.field = "course_code";
    c1.category = "course_record";
    c1.final.value = "BUSI 499";
    c1.original.source.page = 1;
    c1.original.source.bbox = {300, 1000, 150, 30};

    EvidenceField cr1;
    cr1.fieldId = "field_credits_1";
    cr1.field = "credits";
    cr1.category = "course_record";
    cr1.structuralClass = StructuralClass::CourseRow;
    cr1.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
    cr1.sourceBlockId = "block_src_credits";
    cr1.labelBlockId = "table_p1_1";
    cr1.final.value = "3";
    cr1.final.confidence = 0.84f;
    cr1.original.source.page = 1;
    cr1.original.source.bbox = {800, 1000, 30, 25};
    cr1.nearbyEvidence = {"Code: BUSI 499", "Credits: 3"};

    // Row 2: ENGL 101, credits = 4, grade = A
    EvidenceField c2;
    c2.fieldId = "field_course_code_2";
    c2.field = "course_code";
    c2.category = "course_record";
    c2.final.value = "ENGL 101";
    c2.original.source.page = 1;
    c2.original.source.bbox = {300, 1050, 150, 30};

    EvidenceField cr2;
    cr2.fieldId = "field_credits_2";
    cr2.field = "credits";
    cr2.category = "course_record";
    cr2.structuralClass = StructuralClass::CourseRow;
    cr2.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
    cr2.sourceBlockId = "block_src_credits";
    cr2.labelBlockId = "table_p1_1";
    cr2.final.value = "4";
    cr2.final.confidence = 0.84f;
    cr2.original.source.page = 1;
    cr2.original.source.bbox = {800, 1050, 30, 25};
    cr2.nearbyEvidence = {"Code: ENGL 101", "Credits: 4"};

    std::vector<EvidenceField> allFields = {c1, cr1, c2, cr2};

    // Analyze cr1: belongs to Row 1 (BUSI 499)
    auto a1 = analyzer.analyzeField(cr1, doc, allFields);
    EXPECT_EQ(a1.status, ResolutionStatus::Deterministic);
    EXPECT_FALSE(a1.llmRequired);
    EXPECT_EQ(cr1.final.value, "3");

    // Analyze cr2: belongs to Row 2 (ENGL 101)
    auto a2 = analyzer.analyzeField(cr2, doc, allFields);
    EXPECT_EQ(a2.status, ResolutionStatus::Deterministic);
    EXPECT_FALSE(a2.llmRequired);
    EXPECT_EQ(cr2.final.value, "4");
}

// Fix #5 - TEST 5: Genuinely ambiguous credit values remain eligible for LLM routing
TEST(ResolutionAnalyzerTests, Test21_Fix5_Test5_GenuinelyAmbiguousCreditValueEligibleForLlmRouting) {
    RouterConfig config;
    ResolutionAnalyzer analyzer(config);
    auto doc = createBaseDoc();

    // Subtest 5A: Non-numeric credit text "3 A" triggers TABLE_COLUMN_AMBIGUITY
    EvidenceField nonNumericCred;
    nonNumericCred.fieldId = "field_credits_1";
    nonNumericCred.field = "credits";
    nonNumericCred.category = "course_record";
    nonNumericCred.final.value = "3 A";
    nonNumericCred.final.confidence = 0.85f;

    std::vector<EvidenceField> listA = {nonNumericCred};
    auto resA = analyzer.analyzeField(nonNumericCred, doc, listA);
    EXPECT_EQ(resA.status, ResolutionStatus::LlmRequired);
    EXPECT_TRUE(resA.llmRequired);
    EXPECT_EQ(resA.problem.type, "TABLE_COLUMN_AMBIGUITY");

    // Subtest 5B: Conflicting credit candidates for the same row trigger CONFLICTING_EVIDENCE
    EvidenceField conflictCred;
    conflictCred.fieldId = "field_credits_2";
    conflictCred.field = "credits";
    conflictCred.category = "course_record";
    conflictCred.structuralClass = StructuralClass::CourseRow;
    conflictCred.final.value = "3";
    conflictCred.final.confidence = 0.90f;
    conflictCred.conflictingCandidates = {"3", "4"};

    std::vector<EvidenceField> listB = {conflictCred};
    auto resB = analyzer.analyzeField(conflictCred, doc, listB);
    EXPECT_EQ(resB.status, ResolutionStatus::LlmRequired);
    EXPECT_TRUE(resB.llmRequired);
    EXPECT_EQ(resB.problem.type, "CONFLICTING_EVIDENCE");

    // Subtest 5C: Corrupted OCR noise "3%#*" in credits triggers MALFORMED_OCR or TABLE_COLUMN_AMBIGUITY
    EvidenceField malformedCred;
    malformedCred.fieldId = "field_credits_3";
    malformedCred.field = "credits";
    malformedCred.category = "course_record";
    malformedCred.structuralClass = StructuralClass::CourseRow;
    malformedCred.final.value = "3%#*";
    malformedCred.final.confidence = 0.60f;

    std::vector<EvidenceField> listC = {malformedCred};
    auto resC = analyzer.analyzeField(malformedCred, doc, listC);
    EXPECT_EQ(resC.status, ResolutionStatus::LlmRequired);
    EXPECT_TRUE(resC.llmRequired);
    EXPECT_TRUE(resC.isAmbiguous);
    EXPECT_EQ(resC.problem.type, "MALFORMED_OCR");
}

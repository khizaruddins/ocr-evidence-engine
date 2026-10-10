#include <gtest/gtest.h>
#include <ocr/evidence/validation/FinalDocumentValidator.hpp>
#include <ocr/evidence/projection/FinalDocumentProjector.hpp>
#include <ocr/evidence/patch/EdrPatchEngine.hpp>

using namespace ocr::evidence;

TEST(FinalValidatorTests, ValidFinalDocumentPassesValidation) {
    EvidenceDocument doc;
    doc.metadata.id = "doc_valid_001";
    doc.metadata.type = "transcript";
    doc.identity["student_name"] = "Sophia Jones";

    std::vector<EvidenceField> fields;
    EvidenceField f;
    f.fieldId = "field_name_1";
    f.field = "student_name";
    f.category = "identity";
    f.isSemanticValid = true;
    f.original.value = "Sophia Jones";
    f.final.value = "Sophia Jones";
    f.final.confidence = 0.98f;
    f.resolutionStatus = ResolutionStatus::OcrConfident;
    fields.push_back(f);

    EdrPatchEngine patchEngine;
    LlmPatchDocument emptyPatch;
    emptyPatch.documentId = doc.metadata.id;
    auto enhancedRes = patchEngine.applyPatches(doc, fields, emptyPatch);

    auto finalDoc = FinalDocumentProjector::project(enhancedRes, doc);
    auto valRes = FinalDocumentValidator::validate(finalDoc, enhancedRes.enhancedEdrJson, doc.metadata.id);

    EXPECT_TRUE(valRes.valid);
    EXPECT_TRUE(valRes.errors.empty());
}

TEST(FinalValidatorTests, RejectDocumentIdMismatch) {
    nlohmann::json finalDoc{
        {"schema_version", "ocr-final-document-0.1"},
        {"document", {
            {"document_id", "doc_wrong_id"},
            {"document_type", "transcript"}
        }}
    };

    nlohmann::json enhancedEdr{
        {"schema_version", "edr-enhanced-0.1"},
        {"document_id", "doc_expected_id"}
    };

    auto valRes = FinalDocumentValidator::validate(finalDoc, enhancedEdr, "doc_expected_id");
    EXPECT_FALSE(valRes.valid);
    ASSERT_FALSE(valRes.errors.empty());
    EXPECT_NE(valRes.errors[0].find("DOCUMENT_ID_MISMATCH"), std::string::npos);
}

TEST(FinalValidatorTests, RejectInvalidSchemaVersion) {
    nlohmann::json finalDoc{
        {"schema_version", "invalid-schema-1.0"},
        {"document", {
            {"document_id", "doc_001"},
            {"document_type", "transcript"}
        }}
    };

    nlohmann::json enhancedEdr{
        {"schema_version", "edr-enhanced-0.1"},
        {"document_id", "doc_001"}
    };

    auto valRes = FinalDocumentValidator::validate(finalDoc, enhancedEdr, "doc_001");
    EXPECT_FALSE(valRes.valid);
}

TEST(FinalValidatorTests, RejectOutOfRangeConfidence) {
    nlohmann::json finalDoc{
        {"schema_version", "ocr-final-document-0.1"},
        {"document", {
            {"document_id", "doc_001"},
            {"document_type", "transcript"}
        }},
        {"confidence", {
            {"overall", 1.5f} // Out of range!
        }}
    };

    nlohmann::json enhancedEdr{
        {"schema_version", "edr-enhanced-0.1"},
        {"document_id", "doc_001"}
    };

    auto valRes = FinalDocumentValidator::validate(finalDoc, enhancedEdr, "doc_001");
    EXPECT_FALSE(valRes.valid);
}

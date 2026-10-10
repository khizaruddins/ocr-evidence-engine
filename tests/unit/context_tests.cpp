#include <gtest/gtest.h>
#include <edmo/evidence/context/TargetedLlmContextBuilder.hpp>

using namespace edmo::evidence;

TEST(TargetedLlmContextBuilderTests, NoTasksWhenAllTrusted) {
    EvidenceDocument doc;
    doc.metadata.id = "doc_test";
    doc.metadata.type = "transcript";

    EvidenceField field;
    field.fieldId = "field_001";
    field.field = "student_name";
    field.final.value = "Sophia Jones";
    field.final.confidence = 0.98f;
    field.final.llmRequired = false;

    auto context = TargetedLlmContextBuilder::buildContext(doc, {field});
    EXPECT_EQ(context["llm_required"], false);
    EXPECT_TRUE(context["tasks"].empty());
    EXPECT_EQ(context["security"]["document_content"], "UNTRUSTED_EVIDENCE");
}

TEST(TargetedLlmContextBuilderTests, GeneratesTasksOnlyForUnresolved) {
    EvidenceDocument doc;
    doc.metadata.id = "doc_test";
    doc.metadata.type = "transcript";

    EvidenceField trustedField;
    trustedField.fieldId = "field_001";
    trustedField.field = "student_name";
    trustedField.final.value = "Sophia Jones";
    trustedField.final.llmRequired = false;

    EvidenceField ambiguousField;
    ambiguousField.fieldId = "field_002";
    ambiguousField.field = "course_title";
    ambiguousField.original.value = "M@TH 101";
    ambiguousField.original.confidence = 0.55f;
    ambiguousField.final.llmRequired = true;
    ambiguousField.nearbyEvidence = {"Code: CS101", "Credits: 3"};

    auto context = TargetedLlmContextBuilder::buildContext(doc, {trustedField, ambiguousField});
    EXPECT_EQ(context["llm_required"], true);
    ASSERT_EQ(context["tasks"].size(), 1);
    EXPECT_EQ(context["tasks"][0]["field"], "course_title");
    EXPECT_EQ(context["tasks"][0]["ocr_value"], "M@TH 101");
}

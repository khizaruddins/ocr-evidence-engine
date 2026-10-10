#include <gtest/gtest.h>
#include <ocr/evidence/routing/ConfidenceRouter.hpp>

using namespace ocr::evidence;

TEST(ConfidenceRouterTests, HighConfidenceTrusted) {
    ConfidenceRouter router;
    EvidenceField field;
    field.field = "course_title";
    field.original.value = "Microeconomics";
    field.final.value = "Microeconomics";
    field.final.confidence = 0.98f;

    auto decision = router.routeField(field);
    EXPECT_TRUE(decision.isTrusted);
    EXPECT_FALSE(decision.llmRequired);
    EXPECT_EQ(decision.fate, EvidenceFate::Preserved);
}

TEST(ConfidenceRouterTests, DeterministicallyCorrectedHighConfidenceTrusted) {
    ConfidenceRouter router;
    EvidenceField field;
    field.field = "course_title";
    field.original.value = "Microecon0mics";
    field.final.value = "Microeconomics";
    field.final.confidence = 0.96f;
    field.correction.applied = true;
    field.correction.confidence = 0.97f;

    auto decision = router.routeField(field);
    EXPECT_TRUE(decision.isTrusted);
    EXPECT_FALSE(decision.llmRequired);
    EXPECT_EQ(decision.fate, EvidenceFate::Corrected);
}

TEST(ConfidenceRouterTests, LowConfidenceUnresolvedRoutesToLlm) {
    ConfidenceRouter router;
    EvidenceField field;
    field.field = "course_title";
    field.original.value = "Calculus I";
    field.final.value = "Calculus I";
    field.final.confidence = 0.65f;
    field.correction.applied = false;

    auto decision = router.routeField(field);
    EXPECT_FALSE(decision.isTrusted);
    EXPECT_TRUE(decision.llmRequired);
    EXPECT_EQ(decision.fate, EvidenceFate::LlmContext);
}

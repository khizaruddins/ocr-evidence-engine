#include <gtest/gtest.h>
#include <edmo/evidence/confidence/ConfidenceEngine.hpp>

using namespace edmo::evidence;

TEST(ConfidenceEngineTests, CalculateFieldConfidenceBasic) {
    ConfidenceEngine engine;
    EvidenceField field;
    field.original.confidence = 0.92f;

    float score = engine.calculateFieldConfidence(field, 0.99f, 0.95f, 0.98f);
    // (0.50 * 0.92) + (0.15 * 0.99) + (0.15 * 0.95) + (0.20 * 0.98) = 0.46 + 0.1485 + 0.1425 + 0.196 = 0.947
    EXPECT_NEAR(score, 0.947f, 0.01f);
}

TEST(ConfidenceEngineTests, LowConfidencePenaltyApplied) {
    ConfidenceEngine engine;
    EvidenceField field;
    field.original.confidence = 0.85f;
    field.original.rawTokens = {"token1", "token2"};
    field.original.lowConfidenceTokenCount = 1;

    float score = engine.calculateFieldConfidence(field, 0.90f, 0.90f, 0.90f);
    // Base: 0.875, Penalty: (1/2) * 0.25 = 0.125 -> Result: ~0.75
    EXPECT_LT(score, 0.85f);
}

TEST(ConfidenceEngineTests, CalculateSummaryStats) {
    ConfidenceEngine engine;
    std::vector<EvidenceToken> tokens = {
        {"t1", "UNIVERSITY", 0.98f, {}, 1, EvidenceFate::Preserved},
        {"t2", "OF", 0.95f, {}, 1, EvidenceFate::Preserved},
        {"t3", "OXFORD", 0.65f, {}, 1, EvidenceFate::Preserved}
    };

    auto summary = engine.calculateSummary(tokens);
    EXPECT_EQ(summary.tokenCount, 3);
    EXPECT_NEAR(summary.minimum, 0.65f, 0.01f);
    EXPECT_NEAR(summary.maximum, 0.98f, 0.01f);
    EXPECT_NEAR(summary.mean, (0.98f + 0.95f + 0.65f) / 3.0f, 0.01f);
    EXPECT_EQ(summary.lowConfidenceCount, 1);
}

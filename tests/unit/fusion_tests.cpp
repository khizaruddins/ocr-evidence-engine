#include <gtest/gtest.h>
#include <ocr/evidence/fusion/EvidenceFusionEngine.hpp>

using namespace ocr::evidence;

TEST(EvidenceFusionTests, SingleFieldFusion) {
    EvidenceFusionEngine fusion;
    EvidenceField field;
    field.original.value = "MATHEMAT1CS";
    field.original.confidence = 0.82f;
    field.correction.applied = true;
    field.correction.value = "MATHEMATICS";
    field.correction.confidence = 0.97f;

    auto result = fusion.fuseField(field, 1.0f, 0.95f, 0.98f, 0.95f);
    // When corrected, formula combines original, correction, context, structural
    EXPECT_GE(result.finalConfidence, 0.92f);
}

TEST(EvidenceFusionTests, MultiOcrHypothesisFusionConsensus) {
    EvidenceFusionEngine fusion;
    MultiOcrInput input;
    input.sources = {
        {"ocr_primary", "MATHEMATICS", 0.91f},
        {"ocr_secondary", "MATHEMATICS", 0.94f}
    };

    auto fused = fusion.fuseMultiOcr(input);
    EXPECT_EQ(fused.value, "MATHEMATICS");
    // Consensus agreement boosts confidence
    EXPECT_GE(fused.confidence, 0.94f);
}

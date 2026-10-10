#include <gtest/gtest.h>
#include <ocr/evidence/fusion/ConfidenceFusionEngine.hpp>

using namespace ocr::evidence;

TEST(PatchFusionTests, FusionV1FormulaCalculation) {
    ConfidenceFusionEngine engine;

    // Weights: ocr: 0.20, llm: 0.60, dictionary: 0.10, context: 0.10
    // ocr: 0.82, llm: 0.96, dict: 1.0, ctx: 1.0
    // 0.20 * 0.82 + 0.60 * 0.96 + 0.10 * 1.0 + 0.10 * 1.0 = 0.164 + 0.576 + 0.20 = 0.940
    float fused = engine.fuseLlmPatchConfidence(0.82f, 0.96f, 1.0f, 1.0f);
    EXPECT_NEAR(fused, 0.94f, 0.001f);
}

TEST(PatchFusionTests, ConfigurableVersionedFusionWeights) {
    FusionPolicy customPolicy;
    customPolicy.version = "fusion-v2";
    customPolicy.weights.ocr = 0.30f;
    customPolicy.weights.llm = 0.50f;
    customPolicy.weights.dictionary = 0.10f;
    customPolicy.weights.context = 0.10f;

    ConfidenceFusionEngine engine(customPolicy);
    EXPECT_EQ(engine.policy().version, "fusion-v2");

    // 0.30 * 0.80 + 0.50 * 0.90 + 0.10 * 1.0 + 0.10 * 1.0 = 0.24 + 0.45 + 0.20 = 0.89
    float fused = engine.fuseLlmPatchConfidence(0.80f, 0.90f, 1.0f, 1.0f);
    EXPECT_NEAR(fused, 0.89f, 0.001f);
}

#include <gtest/gtest.h>
#include <ocr/evidence/confidence/ConfidenceComparator.hpp>

using namespace ocr::evidence;

TEST(ComparatorTests, FloatPrecisionHighConfidenceCheck) {
    float threshold = 0.95f;
    float epsilon = 0.001f;

    // 0.949999988079071f is float representation for numbers close to 0.95
    float f1 = 0.949999988f;
    EXPECT_TRUE(ConfidenceComparator::atOrAbove(f1, threshold, epsilon));
    EXPECT_FALSE(ConfidenceComparator::below(f1, threshold, epsilon));

    float f2 = 0.95000001f;
    EXPECT_TRUE(ConfidenceComparator::atOrAbove(f2, threshold, epsilon));

    float f3 = 0.9495f;
    EXPECT_TRUE(ConfidenceComparator::atOrAbove(f3, threshold, epsilon));

    // Genuinely low confidence (0.948 < 0.949)
    float f4 = 0.9480f;
    EXPECT_FALSE(ConfidenceComparator::atOrAbove(f4, threshold, epsilon));
    EXPECT_TRUE(ConfidenceComparator::below(f4, threshold, epsilon));
}

TEST(ComparatorTests, EffectivelyEqualWithinEpsilon) {
    EXPECT_TRUE(ConfidenceComparator::effectivelyEqual(0.9500f, 0.9495f, 0.001f));
    EXPECT_FALSE(ConfidenceComparator::effectivelyEqual(0.9500f, 0.9450f, 0.001f));
}

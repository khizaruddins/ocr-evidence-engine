#include <gtest/gtest.h>
#include <ocr/evidence/fuzzy/FuzzyMatcher.hpp>

using namespace ocr::evidence;

TEST(FuzzyMatcherTests, NormalizedLevenshteinIdentical) {
    EXPECT_FLOAT_EQ(FuzzyMatcher::normalizedLevenshtein("MATHEMATICS", "MATHEMATICS"), 1.0f);
    EXPECT_FLOAT_EQ(FuzzyMatcher::normalizedLevenshtein("mathematics", "MATHEMATICS"), 1.0f);
}

TEST(FuzzyMatcherTests, NormalizedLevenshteinOneEdit) {
    // MATHEMAT1CS vs MATHEMATICS: length 11, distance 1 -> similarity 10/11 = 0.909
    float sim = FuzzyMatcher::normalizedLevenshtein("MATHEMAT1CS", "MATHEMATICS");
    EXPECT_GT(sim, 0.90f);
}

TEST(FuzzyMatcherTests, JaroWinklerCalculation) {
    float jw = FuzzyMatcher::jaroWinkler("OXFORD", "OXF0RD");
    EXPECT_GT(jw, 0.85f);
}

TEST(FuzzyMatcherTests, ShortTokenSafeguards) {
    EXPECT_FALSE(FuzzyMatcher::isEligibleForFuzzyMatch("IN"));
    EXPECT_FALSE(FuzzyMatcher::isEligibleForFuzzyMatch("A"));
    EXPECT_FALSE(FuzzyMatcher::isEligibleForFuzzyMatch("3"));
    EXPECT_TRUE(FuzzyMatcher::isEligibleForFuzzyMatch("MATH"));
}

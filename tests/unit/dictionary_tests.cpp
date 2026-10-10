#include <gtest/gtest.h>
#include <edmo/evidence/dictionary/DictionaryEngine.hpp>

using namespace edmo::evidence;

TEST(DictionaryEngineTests, DefaultVocabulariesLoaded) {
    DictionaryEngine dict;

    EXPECT_TRUE(dict.contains("TRANSCRIPT", DictionaryType::Transcript));
    EXPECT_TRUE(dict.contains("transcript", DictionaryType::Transcript));
    EXPECT_TRUE(dict.contains("MATHEMATICS", DictionaryType::Academic));
    EXPECT_TRUE(dict.contains("mathematics", DictionaryType::Academic));
    EXPECT_TRUE(dict.contains("BACHELORS", DictionaryType::Degrees));
    EXPECT_TRUE(dict.containsAny("UNIVERSITY"));
}

TEST(DictionaryEngineTests, GradeValidation) {
    DictionaryEngine dict;

    EXPECT_TRUE(dict.isValidGrade("A+"));
    EXPECT_TRUE(dict.isValidGrade("a"));
    EXPECT_TRUE(dict.isValidGrade("B"));
    EXPECT_TRUE(dict.isValidGrade("P"));
    EXPECT_TRUE(dict.isValidGrade("W"));
    EXPECT_FALSE(dict.isValidGrade("XYZ"));
}

TEST(DictionaryEngineTests, ScoreTermQuality) {
    DictionaryEngine dict;

    EXPECT_FLOAT_EQ(dict.scoreTerm("TRANSCRIPT"), 1.0f);
    EXPECT_FLOAT_EQ(dict.scoreTerm("ACCOUNTING"), 0.98f);
    EXPECT_FLOAT_EQ(dict.scoreTerm("UNKNOWN_NONWORD_123"), 0.0f);
}

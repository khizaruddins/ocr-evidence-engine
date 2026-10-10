#include <gtest/gtest.h>
#include <edmo/evidence/normalization/FieldValueNormalizer.hpp>

using namespace edmo::evidence;

TEST(NormalizerTests, NormalizeStudentIdWithPrefixAndColon) {
    auto res1 = FieldValueNormalizer::normalize("student_id", ": 712398456", "STUDENT ID");
    EXPECT_EQ(res1.normalizedValue, "712398456");
    EXPECT_TRUE(res1.applied);

    auto res2 = FieldValueNormalizer::normalize("student_id", "Student ID: 712398456");
    EXPECT_EQ(res2.normalizedValue, "712398456");
    EXPECT_TRUE(res2.applied);

    auto res3 = FieldValueNormalizer::normalize("student_id", "Student ID - 712398456");
    EXPECT_EQ(res3.normalizedValue, "712398456");
    EXPECT_TRUE(res3.applied);

    auto res4 = FieldValueNormalizer::normalize("student_id", "  GU-2024-9182  ");
    EXPECT_EQ(res4.normalizedValue, "GU-2024-9182");
    EXPECT_TRUE(res4.applied);
}

TEST(NormalizerTests, NormalizeCourseCodeSpacing) {
    auto res = FieldValueNormalizer::normalize("course_code", "ECON   001");
    EXPECT_EQ(res.normalizedValue, "ECON 001");
    EXPECT_TRUE(res.applied);
}

TEST(NormalizerTests, NormalizeCourseTitlePreservesTitleFormatting) {
    auto res = FieldValueNormalizer::normalize("course_title", "  Prin of Microeconomics  ");
    EXPECT_EQ(res.normalizedValue, "Prin of Microeconomics");
    EXPECT_TRUE(res.applied);
}

TEST(NormalizerTests, NormalizeGradePreservesGrade) {
    auto res1 = FieldValueNormalizer::normalize("grade", ": A-");
    EXPECT_EQ(res1.normalizedValue, "A-");
    EXPECT_TRUE(res1.applied);

    auto res2 = FieldValueNormalizer::normalize("grade", "In Progress");
    EXPECT_EQ(res2.normalizedValue, "In Progress");
}

TEST(NormalizerTests, GenericDateNormalizationRequiredFormats) {
    // 1. Standard text date
    EXPECT_EQ(FieldValueNormalizer::normalize("birthdate", "Jun 26 1997").normalizedValue, "Jun 26 1997");

    // 2. Merged day and 4-digit year without space
    EXPECT_EQ(FieldValueNormalizer::normalize("birthdate", "Jun 261997").normalizedValue, "Jun 26 1997");

    // 3. Full month name with space
    EXPECT_EQ(FieldValueNormalizer::normalize("birthdate", "June 26 1997").normalizedValue, "Jun 26 1997");

    // 4. Full month name merged day and 4-digit year without space
    EXPECT_EQ(FieldValueNormalizer::normalize("birthdate", "June 261997").normalizedValue, "Jun 26 1997");

    // 5. Month name with comma
    EXPECT_EQ(FieldValueNormalizer::normalize("birthdate", "Jun 26, 1997").normalizedValue, "Jun 26 1997");

    // 6. Numeric slash format MM/DD/YYYY
    EXPECT_EQ(FieldValueNormalizer::normalize("birthdate", "06/26/1997").normalizedValue, "Jun 26 1997");

    // 7. Numeric hyphen format MM-DD-YYYY
    EXPECT_EQ(FieldValueNormalizer::normalize("birthdate", "06-26-1997").normalizedValue, "Jun 26 1997");

    // 8. ISO format YYYY-MM-DD
    EXPECT_EQ(FieldValueNormalizer::normalize("birthdate", "1997-06-26").normalizedValue, "Jun 26 1997");

    // 9. Day first DMY format
    EXPECT_EQ(FieldValueNormalizer::normalize("birthdate", "26 Jun 1997").normalizedValue, "Jun 26 1997");

    // 10. Date with label prefix and colon
    EXPECT_EQ(FieldValueNormalizer::normalize("birthdate", "BIRTHDATE : Jun 261997", "BIRTHDATE").normalizedValue, "Jun 26 1997");
    EXPECT_EQ(FieldValueNormalizer::normalize("birthdate", "DOB: 06/26/1997").normalizedValue, "Jun 26 1997");
    EXPECT_EQ(FieldValueNormalizer::normalize("conferral_date", "DATE CONFERRED: June 26, 1997").normalizedValue, "Jun 26 1997");

    // 11. Generic dates across different months to ensure no hardcoding
    EXPECT_EQ(FieldValueNormalizer::normalize("date", "Jan 15 2001").normalizedValue, "Jan 15 2001");
    EXPECT_EQ(FieldValueNormalizer::normalize("date", "October 31, 1998").normalizedValue, "Oct 31 1998");
    EXPECT_EQ(FieldValueNormalizer::normalize("date", "Dec 252020").normalizedValue, "Dec 25 2020");
}


#include <gtest/gtest.h>
#include <ocr/evidence/correction/CorrectionEngine.hpp>

using namespace ocr::evidence;

TEST(CorrectionEngineTests, CorrectCourseTitleCharacterConfusion) {
    CorrectionEngine engine;
    auto result = engine.correctText("MATHEMAT1CS FOR C0MPUTING", "course_title", 0.82f);

    EXPECT_TRUE(result.applied);
    EXPECT_EQ(result.value, "MATHEMATICS FOR COMPUTING");
    EXPECT_GT(result.confidence, 0.90f);
}

TEST(CorrectionEngineTests, CorrectUniversityConfusion) {
    CorrectionEngine engine;
    auto result = engine.correctText("UN1VERSITY OF OXF0RD", "institution_name", 0.80f);

    EXPECT_TRUE(result.applied);
    EXPECT_EQ(result.value, "UNIVERSITY OF OXFORD");
    EXPECT_GE(result.confidence, 0.90f);
}

TEST(CorrectionEngineTests, CourseCodeSafeguardDoesNotCorruptCourseNumbers) {
    CorrectionEngine engine;
    // "1CS101" or "CS101" must NOT have numeric parts converted to letters!
    auto result = engine.correctText("CS101", "course_code", 0.95f);
    EXPECT_EQ(result.value, "CS101");

    auto result2 = engine.correctText("ACCT 101", "course_code", 0.92f);
    EXPECT_EQ(result2.value, "ACCT 101");
}

TEST(CorrectionEngineTests, CorrectCreditsMisread) {
    CorrectionEngine engine;
    auto resultO = engine.correctText("O", "credits", 0.80f);
    EXPECT_EQ(resultO.value, "0");

    auto resultI = engine.correctText("I", "credits", 0.80f);
    EXPECT_EQ(resultI.value, "1");

    auto resultValid = engine.correctText("3.0", "credits", 0.98f);
    EXPECT_EQ(resultValid.value, "3.0");
}

TEST(CorrectionEngineTests, CorrectGradeMisread) {
    CorrectionEngine engine;
    auto result = engine.correctText("A", "grade", 0.95f);
    EXPECT_EQ(result.value, "A");

    auto resultZeroToO = engine.correctText("0", "grade", 0.80f);
    EXPECT_EQ(resultZeroToO.value, "O");
}

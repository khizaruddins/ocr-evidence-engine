#include <gtest/gtest.h>
#include <ocr/evidence/structure/PlaceholderDetector.hpp>
#include <ocr/evidence/structure/StructuralClassifier.hpp>
#include <ocr/evidence/structure/CourseRowDetector.hpp>
#include <ocr/evidence/structure/FieldCandidateValidator.hpp>

using namespace ocr::evidence;

TEST(StructureTests, PlaceholderAndSeparatorDetection) {
    EXPECT_TRUE(PlaceholderDetector::isSeparatorLine("-----------------"));
    EXPECT_TRUE(PlaceholderDetector::isSeparatorLine("_________________"));
    EXPECT_TRUE(PlaceholderDetector::isSeparatorLine("================="));
    EXPECT_TRUE(PlaceholderDetector::isSeparatorLine("..."));
    EXPECT_TRUE(PlaceholderDetector::isSeparatorLine(""));

    EXPECT_TRUE(PlaceholderDetector::isPlaceholder("-----------------"));
    EXPECT_TRUE(PlaceholderDetector::isPlaceholder("N/A"));
    EXPECT_TRUE(PlaceholderDetector::isPlaceholder("NA"));
    EXPECT_TRUE(PlaceholderDetector::isPlaceholder("null"));
    EXPECT_TRUE(PlaceholderDetector::isPlaceholder("NULL"));
    EXPECT_TRUE(PlaceholderDetector::isPlaceholder("unknown"));
    EXPECT_TRUE(PlaceholderDetector::isPlaceholder("-"));
    EXPECT_TRUE(PlaceholderDetector::isPlaceholder("--"));
    EXPECT_TRUE(PlaceholderDetector::isPlaceholder("---"));
    EXPECT_TRUE(PlaceholderDetector::isPlaceholder(""));

    // Legitimate course codes with dashes must NOT be treated as placeholders
    EXPECT_FALSE(PlaceholderDetector::isPlaceholder("CS-106A"));
    EXPECT_FALSE(PlaceholderDetector::isPlaceholder("BIO-101"));
    EXPECT_FALSE(PlaceholderDetector::isPlaceholder("MATH-51"));
    EXPECT_FALSE(PlaceholderDetector::isPlaceholder("ECON 001"));
}

TEST(StructureTests, LabeledPlaceholderDetection) {
    EXPECT_TRUE(PlaceholderDetector::isLabeledPlaceholder("Code: -----------------"));
    EXPECT_TRUE(PlaceholderDetector::isLabeledPlaceholder("Grade: N/A"));
    EXPECT_TRUE(PlaceholderDetector::isLabeledPlaceholder("Credits: ---"));
    EXPECT_FALSE(PlaceholderDetector::isLabeledPlaceholder("Code: ECON 001"));
}

TEST(StructureTests, StructuralClassification) {
    StructuralClassifier classifier;

    EXPECT_EQ(classifier.classifyText("-----------------"), StructuralClass::TableSeparator);
    EXPECT_EQ(classifier.classifyText("----------------- Fall 2024 -----------------"), StructuralClass::TermHeader);
    EXPECT_EQ(classifier.classifyText("Fall 2024"), StructuralClass::TermHeader);
    EXPECT_EQ(classifier.classifyText("Spring 2025"), StructuralClass::TermHeader);
    EXPECT_EQ(classifier.classifyText("Subj Crs Title Crd Grd Pts R"), StructuralClass::TableHeader);
    EXPECT_EQ(classifier.classifyText("Course Code Course Title Credits Grade Points"), StructuralClass::TableHeader);
    EXPECT_EQ(classifier.classifyText("In Progress:"), StructuralClass::CourseStatusHeader);
    EXPECT_EQ(classifier.classifyText("First Honors"), StructuralClass::AcademicDistinction);
    EXPECT_EQ(classifier.classifyText("Dean's List"), StructuralClass::AcademicDistinction);
    EXPECT_EQ(classifier.classifyText("Transcript Totals:"), StructuralClass::SummaryHeader);
    EXPECT_EQ(classifier.classifyText("Current 7.00 7.00 26.68 3.811"), StructuralClass::SummaryRow);
    EXPECT_EQ(classifier.classifyText("Cumulative 46.00 46.00 181.20 3.940"), StructuralClass::SummaryRow);
    EXPECT_EQ(classifier.classifyText("GEORGETOWN UNIVERSITY"), StructuralClass::InstitutionData);
    EXPECT_EQ(classifier.classifyText("Page 1 of 2"), StructuralClass::PageNumber);
    EXPECT_EQ(classifier.classifyText("ECON 001 Prin of Microeconomics 3.00 A 12.00"), StructuralClass::CourseRow);
}

TEST(StructureTests, TermNameExtraction) {
    StructuralClassifier classifier;
    EXPECT_EQ(classifier.extractTermName("----------------- Fall 2024 -----------------"), "Fall 2024");
    EXPECT_EQ(classifier.extractTermName("Spring 2025"), "Spring 2025");
    EXPECT_EQ(classifier.extractTermName("Fall Semester 2024"), "Fall Semester 2024");
}

TEST(StructureTests, CourseCodeValidation) {
    CourseRowDetector detector;
    EXPECT_TRUE(detector.isValidCourseCode("ECON 001"));
    EXPECT_TRUE(detector.isValidCourseCode("MATH 035"));
    EXPECT_TRUE(detector.isValidCourseCode("CS 101"));
    EXPECT_TRUE(detector.isValidCourseCode("CS-106A"));
    EXPECT_TRUE(detector.isValidCourseCode("22BS103"));
    EXPECT_TRUE(detector.isValidCourseCode("BIO-101"));
    EXPECT_TRUE(detector.isValidCourseCode("CHEM 102A"));

    EXPECT_FALSE(detector.isValidCourseCode("-----------------"));
    EXPECT_FALSE(detector.isValidCourseCode("Fall 2024"));
    EXPECT_FALSE(detector.isValidCourseCode("Current"));
    EXPECT_FALSE(detector.isValidCourseCode("Dean's List"));
    EXPECT_FALSE(detector.isValidCourseCode("N/A"));
}

TEST(StructureTests, CourseRowDecompositionStandard) {
    CourseRowDetector detector;
    auto res = detector.parseText("ECON 001 Prin of Microeconomics 3.00 A 12.00", 0.98f, "Fall 2024");

    EXPECT_TRUE(res.isValid);
    EXPECT_EQ(res.courseCode, "ECON 001");
    EXPECT_EQ(res.courseTitle, "Prin of Microeconomics");
    EXPECT_EQ(res.credits, "3.00");
    EXPECT_EQ(res.grade, "A");
    EXPECT_EQ(res.qualityPoints, "12.00");
    EXPECT_EQ(res.status, "COMPLETED");
    EXPECT_EQ(res.term, "Fall 2024");
}

TEST(StructureTests, CourseRowDecompositionMinusGrade) {
    CourseRowDetector detector;
    auto res = detector.parseText("MATH 035 Calculus I 4.00 A- 14.68", 0.98f, "Fall 2024");

    EXPECT_TRUE(res.isValid);
    EXPECT_EQ(res.courseCode, "MATH 035");
    EXPECT_EQ(res.courseTitle, "Calculus I");
    EXPECT_EQ(res.credits, "4.00");
    EXPECT_EQ(res.grade, "A-");
    EXPECT_EQ(res.qualityPoints, "14.68");
    EXPECT_EQ(res.status, "COMPLETED");
}

TEST(StructureTests, CourseRowDecompositionInProgress) {
    CourseRowDetector detector;
    auto res = detector.parseText("ECON 002 Prin of Macroeconomics 3.00 In Progress", 0.98f, "Spring 2025");

    EXPECT_TRUE(res.isValid);
    EXPECT_EQ(res.courseCode, "ECON 002");
    EXPECT_EQ(res.courseTitle, "Prin of Macroeconomics");
    EXPECT_EQ(res.credits, "3.00");
    EXPECT_EQ(res.grade, ""); // Grade must NOT be "In Progress"
    EXPECT_EQ(res.status, "IN_PROGRESS");
    EXPECT_EQ(res.term, "Spring 2025");
}

TEST(StructureTests, FieldCandidateValidationGate) {
    FieldCandidateValidator validator;

    // 1. Separator candidate is invalid
    EvidenceField fSep;
    fSep.field = "course_code";
    fSep.original.value = "-----------------";
    fSep.structuralClass = StructuralClass::TableSeparator;
    auto vSep = validator.validateCandidate(fSep);
    EXPECT_FALSE(vSep.isValid);
    EXPECT_FALSE(vSep.llmEligible);

    // 2. Term candidate is invalid for course title
    EvidenceField fTerm;
    fTerm.field = "course_title";
    fTerm.original.value = "Fall 2024 -----------------";
    fTerm.structuralClass = StructuralClass::TermHeader;
    auto vTerm = validator.validateCandidate(fTerm);
    EXPECT_FALSE(vTerm.isValid);
    EXPECT_FALSE(vTerm.llmEligible);

    // 3. Distinction is invalid for course title
    EvidenceField fDist;
    fDist.field = "course_title";
    fDist.original.value = "First Honors";
    fDist.structuralClass = StructuralClass::AcademicDistinction;
    auto vDist = validator.validateCandidate(fDist);
    EXPECT_FALSE(vDist.isValid);
    EXPECT_FALSE(vDist.llmEligible);

    // 4. Valid course title
    EvidenceField fTitle;
    fTitle.field = "course_title";
    fTitle.original.value = "Prin of Microeconomics";
    fTitle.structuralClass = StructuralClass::CourseRow;
    auto vTitle = validator.validateCandidate(fTitle);
    EXPECT_TRUE(vTitle.isValid);
    EXPECT_TRUE(vTitle.llmEligible);
}

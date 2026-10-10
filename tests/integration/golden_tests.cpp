#include <gtest/gtest.h>
#include <edmo/evidence/pipeline/EvidencePipeline.hpp>
#include <edmo/evidence/patch/EdrPatchEngine.hpp>
#include <edmo/evidence/projection/FinalDocumentProjector.hpp>

#include <filesystem>

using namespace edmo::evidence;

TEST(GoldenTests, ProcessRealSophiaJonesTranscriptIfAvailable) {
    std::vector<std::filesystem::path> candidates = {
        "/home/khizaruddin/projects/fullapp/EDMO-OCR-DI/EDMO C++ OCR Engine/OCR Layer/output/documents/Sophia_Jones_Transcript/edr_full.json",
        "examples/input/sophia_jones_edr.json",
        "../examples/input/sophia_jones_edr.json"
    };

    std::filesystem::path target;
    for (const auto& cand : candidates) {
        if (std::filesystem::exists(cand)) {
            target = cand;
            break;
        }
    }

    if (target.empty()) {
        GTEST_SKIP() << "Sophia Jones golden test file not found on disk";
    }

    PipelineConfig config;
    EvidencePipeline pipeline(config);
    auto result = pipeline.processFile(target);

    EXPECT_FALSE(result.fields.empty());
    EXPECT_EQ(result.zeroLossReport.unaccountedLoss, 0);
    EXPECT_FALSE(result.llmRequired);
    EXPECT_EQ(result.llmTaskCount, 0);

    // Apply forward deterministic results to projection
    EdrPatchEngine patchEngine;
    LlmPatchDocument emptyPatchDoc;
    emptyPatchDoc.documentId = result.document.metadata.id;
    auto enhancedResult = patchEngine.applyPatches(result.document, result.fields, emptyPatchDoc);
    auto finalDoc = FinalDocumentProjector::project(enhancedResult, result.document);

    // 1. Assert Identity and Normalized Birthdate
    ASSERT_TRUE(finalDoc.contains("student"));
    EXPECT_EQ(finalDoc["student"]["name"], "Sophia Jones");
    EXPECT_EQ(finalDoc["student"]["student_name"], "Sophia Jones");
    EXPECT_EQ(finalDoc["student"]["student_id"], "2291829");
    EXPECT_EQ(finalDoc["student"]["birthdate"], "Jun 26 1997");

    // 2. Assert Program Department and Major
    ASSERT_TRUE(finalDoc.contains("program"));
    EXPECT_EQ(finalDoc["program"]["department"], "Accounting");
    EXPECT_EQ(finalDoc["program"]["major"], "Accounting & Finance");

    // 3. Assert All 15 Expected Course Rows
    ASSERT_TRUE(finalDoc.contains("courses"));
    ASSERT_TRUE(finalDoc["courses"].is_array());
    const auto& courses = finalDoc["courses"];
    EXPECT_GE(courses.size(), 15);

    struct ExpectedCourse {
        std::string code;
        double credits;
        std::string grade;
    };

    const std::vector<ExpectedCourse> expectedCourses = {
        {"ACCT 101", 3.0, "A"},
        {"ACCT 102", 3.0, "A"},
        {"FINC 201", 3.0, "A"},
        {"BUSI 203", 3.0, "A"},
        {"BUSI 101", 3.0, "A"},
        {"ACCT 301", 3.0, "B"},
        {"ACCT 401", 3.0, "C"},
        {"ACCT 402", 3.0, "A"},
        {"FINC 301", 3.0, "B"},
        {"ACCT 601", 3.0, "B"},
        {"BUSI 499", 3.0, "B"},
        {"AUDT 301", 3.0, "B"},
        {"MIS 301", 3.0, "A"},
        {"LAW 301", 2.0, "C"},
        {"TAX 301", 3.0, "C"}
    };

    std::map<std::string, const nlohmann::json*> foundCourses;
    for (const auto& crs : courses) {
        // Assert: course_code != null, course_title != null, credits != null
        EXPECT_FALSE(crs["course_code"].is_null());
        EXPECT_FALSE(crs["course_title"].is_null());
        EXPECT_FALSE(crs["credits"].is_null());

        // When grade is visible in the source, grade != null
        EXPECT_FALSE(crs["grade"].is_null());
        EXPECT_FALSE(crs["grade"].get<std::string>().empty());

        if (crs.contains("course_code") && !crs["course_code"].is_null()) {
            foundCourses[crs["course_code"].get<std::string>()] = &crs;
        }
    }

    for (const auto& exp : expectedCourses) {
        ASSERT_TRUE(foundCourses.count(exp.code) > 0) << "Missing expected course: " << exp.code;
        const auto& crs = *foundCourses[exp.code];
        EXPECT_DOUBLE_EQ(crs["credits"].get<double>(), exp.credits) << "Credits mismatch for " << exp.code;
        EXPECT_EQ(crs["grade"].get<std::string>(), exp.grade) << "Grade mismatch for " << exp.code;
    }

    // Specific BUSI 499 test (Acceptance Criteria #9)
    ASSERT_TRUE(foundCourses.count("BUSI 499") > 0);
    const auto& busi499 = *foundCourses["BUSI 499"];
    EXPECT_EQ(busi499["course_code"], "BUSI 499");
    EXPECT_EQ(busi499["course_title"], "Personal and Professional Development");
    EXPECT_DOUBLE_EQ(busi499["credits"].get<double>(), 3.0);
    EXPECT_EQ(busi499["grade"], "B");
}

TEST(GoldenTests, StructuredCourseGradeDataLossInvariant) {
    // Deterministic Invariant:
    // If a structured table row contains a grade cell and that grade is valid/recognized,
    // then final projection MUST NOT silently convert it to null.
    nlohmann::json edrJson = {
        {"document", {
            {"id", "doc_structured_grades_test"},
            {"type", "transcript"},
            {"page_count", 1}
        }},
        {"tables", nlohmann::json::array({
            {
                {"id", "table_p1_1"},
                {"page_start", 1},
                {"page_end", 1},
                {"has_header", true},
                {"rows", nlohmann::json::array({
                    {
                        {"cells", nlohmann::json::array({
                            {{"text", "ACCT 101"}, {"canonical_name", "course_code"}, {"fate", "STRUCTURED"}},
                            {{"text", "Intro to Accounting"}, {"canonical_name", "course_title"}, {"fate", "STRUCTURED"}},
                            {{"text", "3"}, {"canonical_name", "credits"}, {"fate", "STRUCTURED"}},
                            {{"text", "A"}, {"canonical_name", "grade"}, {"fate", "STRUCTURED"}}
                        })}
                    },
                    {
                        {"cells", nlohmann::json::array({
                            {{"text", "BUSI 499"}, {"canonical_name", "course_code"}, {"fate", "STRUCTURED"}},
                            {{"text", "Personal and Professional Development"}, {"canonical_name", "course_title"}, {"fate", "STRUCTURED"}},
                            {{"text", "3"}, {"canonical_name", "credits"}, {"fate", "STRUCTURED"}},
                            {{"text", "B"}, {"canonical_name", "grade"}, {"fate", "STRUCTURED"}}
                        })}
                    },
                    {
                        {"cells", nlohmann::json::array({
                            {{"text", "FINC 201"}, {"canonical_name", "course_code"}, {"fate", "STRUCTURED"}},
                            {{"text", "Raising Individual Financial Awareness"}, {"canonical_name", "course_title"}, {"fate", "STRUCTURED"}},
                            {{"text", "3"}, {"canonical_name", "credits"}, {"fate", "STRUCTURED"}},
                            {{"text", "A"}, {"canonical_name", "grade"}, {"fate", "STRUCTURED"}}
                        })}
                    },
                    {
                        {"cells", nlohmann::json::array({
                            {{"text", "MIS 301"}, {"canonical_name", "course_code"}, {"fate", "STRUCTURED"}},
                            {{"text", "Management Information Systems"}, {"canonical_name", "course_title"}, {"fate", "STRUCTURED"}},
                            {{"text", "3"}, {"canonical_name", "credits"}, {"fate", "STRUCTURED"}},
                            {{"text", "A"}, {"canonical_name", "grade"}, {"fate", "STRUCTURED"}}
                        })}
                    }
                })}
            }
        })}
    };

    PipelineConfig config;
    EvidencePipeline pipeline(config);
    auto result = pipeline.process(edrJson);

    EdrPatchEngine patchEngine;
    LlmPatchDocument emptyPatch;
    emptyPatch.documentId = result.document.metadata.id;
    auto enhancedResult = patchEngine.applyPatches(result.document, result.fields, emptyPatch);
    auto finalDoc = FinalDocumentProjector::project(enhancedResult, result.document);

    ASSERT_TRUE(finalDoc.contains("courses"));
    const auto& courses = finalDoc["courses"];
    EXPECT_GE(courses.size(), 4);

    for (const auto& crs : courses) {
        EXPECT_FALSE(crs["course_code"].is_null());
        EXPECT_FALSE(crs["course_title"].is_null());
        EXPECT_FALSE(crs["credits"].is_null());
        EXPECT_FALSE(crs["grade"].is_null());
        std::string gradeStr = crs["grade"];
        EXPECT_FALSE(gradeStr.empty());

        if (crs["course_code"] == "BUSI 499") {
            EXPECT_EQ(crs["course_title"], "Personal and Professional Development");
            EXPECT_EQ(crs["credits"], 3.0);
            EXPECT_EQ(crs["grade"], "B");
        } else if (crs["course_code"] == "ACCT 101") {
            EXPECT_EQ(crs["grade"], "A");
        } else if (crs["course_code"] == "FINC 201") {
            EXPECT_EQ(crs["grade"], "A");
        } else if (crs["course_code"] == "MIS 301") {
            EXPECT_EQ(crs["grade"], "A");
        }
    }
}

TEST(GoldenTests, StructuredCourseCreditsDataLossInvariant) {
    // Deterministic Invariant for Credits:
    // If a structured table row contains a credits cell and that credit is valid,
    // then deterministic resolution and final projection MUST NOT mark it AMBIGUOUS_FIELD_VALUE
    // or convert it to null.
    nlohmann::json edrJson = {
        {"document", {
            {"id", "doc_structured_credits_test"},
            {"type", "transcript"},
            {"page_count", 1}
        }},
        {"tables", nlohmann::json::array({
            {
                {"id", "table_p1_1"},
                {"page_start", 1},
                {"page_end", 1},
                {"has_header", true},
                {"columns", nlohmann::json::array({
                    {{"name", "course_code"}, {"canonical_name", "course_code"}},
                    {{"name", "course_title"}, {"canonical_name", "course_title"}},
                    {{"name", "credits"}, {"canonical_name", "credits"}},
                    {{"name", "grade"}, {"canonical_name", "grade"}}
                })},
                {"rows", nlohmann::json::array({
                    {
                        {"cells", nlohmann::json::array({
                            {{"text", "BUSI 499"}, {"canonical_name", "course_code"}, {"fate", "STRUCTURED"}},
                            {{"text", "Personal and Professional Development"}, {"canonical_name", "course_title"}, {"fate", "STRUCTURED"}},
                            {{"text", "3"}, {"canonical_name", "credits"}, {"fate", "STRUCTURED"}, {"confidence", 0.84608f}},
                            {{"text", "B"}, {"canonical_name", "grade"}, {"fate", "STRUCTURED"}}
                        })},
                        {"canonical_fields", {
                            {"course_code", "BUSI 499"},
                            {"course_title", "Personal and Professional Development"},
                            {"credits", "3"},
                            {"grade", "B"}
                        }},
                        {"confidence", 0.90f}
                    },
                    {
                        {"cells", nlohmann::json::array({
                            {{"text", "ACCT 101"}, {"canonical_name", "course_code"}, {"fate", "STRUCTURED"}},
                            {{"text", "Intro to Accounting"}, {"canonical_name", "course_title"}, {"fate", "STRUCTURED"}},
                            {{"text", "3"}, {"canonical_name", "credits"}, {"fate", "STRUCTURED"}, {"confidence", 0.82f}},
                            {{"text", "A"}, {"canonical_name", "grade"}, {"fate", "STRUCTURED"}}
                        })},
                        {"canonical_fields", {
                            {"course_code", "ACCT 101"},
                            {"course_title", "Intro to Accounting"},
                            {"credits", "3"},
                            {"grade", "A"}
                        }},
                        {"confidence", 0.92f}
                    }
                })}
            }
        })}
    };

    PipelineConfig config;
    EvidencePipeline pipeline(config);
    auto result = pipeline.process(edrJson);

    EXPECT_FALSE(result.llmRequired);
    EXPECT_EQ(result.llmTaskCount, 0);

    // Apply forward deterministic results to projection
    EdrPatchEngine patchEngine;
    LlmPatchDocument emptyPatchDoc;
    emptyPatchDoc.documentId = result.document.metadata.id;
    auto enhancedResult = patchEngine.applyPatches(result.document, result.fields, emptyPatchDoc);
    auto finalDoc = FinalDocumentProjector::project(enhancedResult, result.document);

    ASSERT_TRUE(finalDoc.contains("courses"));
    const auto& courses = finalDoc["courses"];
    ASSERT_EQ(courses.size(), 2);

    for (const auto& crs : courses) {
        EXPECT_FALSE(crs["credits"].is_null());
        EXPECT_EQ(crs["credits"], 3.0);
        EXPECT_FALSE(crs["grade"].is_null());
        if (crs["course_code"] == "BUSI 499") {
            EXPECT_EQ(crs["course_title"], "Personal and Professional Development");
            EXPECT_EQ(crs["grade"], "B");
        }
    }
}



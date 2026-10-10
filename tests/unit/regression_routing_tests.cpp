#include <gtest/gtest.h>
#include <edmo/evidence/pipeline/EvidencePipeline.hpp>
#include <nlohmann/json.hpp>

using namespace edmo::evidence;

namespace {

nlohmann::json createGeorgetownTranscriptEdr() {
    return nlohmann::json{
        {"schema_version", "edr-0.2"},
        {"document", {
            {"id", "doc_georgetown_test"},
            {"type", "transcript"},
            {"page_count", 2}
        }},
        {"engine", {
            {"name", "edmo-ocr"},
            {"version", "0.2.0"}
        }},
        {"identity", {
            {"student_name", "Marcus J. Thorne"},
            {"student_id", "GU-2024-9182"}
        }},
        {"institution", {
            {"name", "GEORGETOWN UNIVERSITY"}
        }},
        {"tables", nlohmann::json::array({
            {
                {"id", "tbl_1"},
                {"page_start", 1},
                {"has_header", true},
                {"columns", nlohmann::json::array({
                    {{"name", "Subj Crs"}, {"canonical_name", "course_code"}},
                    {{"name", "Title"}, {"canonical_name", "course_title"}},
                    {{"name", "Crd"}, {"canonical_name", "credits"}},
                    {{"name", "Grd"}, {"canonical_name", "grade"}},
                    {{"name", "Pts"}, {"canonical_name", "quality_points"}}
                })},
                {"rows", nlohmann::json::array({
                    // Row 0: Table Header
                    {{"is_header", true}, {"confidence", 0.99}, {"cells", nlohmann::json::array({
                        {{"text", "Subj Crs"}, {"confidence", 0.99}},
                        {{"text", "Title"}, {"confidence", 0.99}},
                        {{"text", "Crd"}, {"confidence", 0.99}},
                        {{"text", "Grd"}, {"confidence", 0.99}},
                        {{"text", "Pts"}, {"confidence", 0.99}}
                    })}},
                    // Row 1: Table Separator
                    {{"is_header", false}, {"confidence", 1.0}, {"cells", nlohmann::json::array({
                        {{"text", "-----------------"}, {"confidence", 1.0}}
                    })}},
                    // Row 2: Term Header
                    {{"is_header", false}, {"confidence", 0.99}, {"cells", nlohmann::json::array({
                        {{"text", "----------------- Fall 2024 -----------------"}, {"confidence", 0.99}}
                    })}},
                    // Row 3: Course 1
                    {{"is_header", false}, {"confidence", 0.98}, {"cells", nlohmann::json::array({
                        {{"text", "ECON 001"}, {"confidence", 0.99}},
                        {{"text", "Prin of Microeconomics"}, {"confidence", 0.98}},
                        {{"text", "3.00"}, {"confidence", 0.99}},
                        {{"text", "A"}, {"confidence", 0.99}},
                        {{"text", "12.00"}, {"confidence", 0.99}}
                    })}},
                    // Row 4: Course 2 (Single concatenated line simulation)
                    {{"is_header", false}, {"confidence", 0.97}, {"cells", nlohmann::json::array({
                        {{"text", "MATH 035 Calculus I 4.00 A- 14.68"}, {"confidence", 0.97}}
                    })}},
                    // Row 5: Distinction
                    {{"is_header", false}, {"confidence", 0.99}, {"cells", nlohmann::json::array({
                        {{"text", "First Honors"}, {"confidence", 0.99}}
                    })}},
                    // Row 6: Term 2
                    {{"is_header", false}, {"confidence", 0.99}, {"cells", nlohmann::json::array({
                        {{"text", "----------------- Spring 2025 -----------------"}, {"confidence", 0.99}}
                    })}},
                    // Row 7: Course Status Header
                    {{"is_header", false}, {"confidence", 0.99}, {"cells", nlohmann::json::array({
                        {{"text", "In Progress:"}, {"confidence", 0.99}}
                    })}},
                    // Row 8: Course 3 (In Progress)
                    {{"is_header", false}, {"confidence", 0.98}, {"cells", nlohmann::json::array({
                        {{"text", "ECON 002"}, {"confidence", 0.99}},
                        {{"text", "Prin of Macroeconomics"}, {"confidence", 0.98}},
                        {{"text", "3.00"}, {"confidence", 0.99}},
                        {{"text", "In Progress"}, {"confidence", 0.99}}
                    })}},
                    // Row 9: Summary Header
                    {{"is_header", false}, {"confidence", 0.99}, {"cells", nlohmann::json::array({
                        {{"text", "Transcript Totals: EHrs QHrs QPts GPA"}, {"confidence", 0.99}}
                    })}},
                    // Row 10: Summary Row
                    {{"is_header", false}, {"confidence", 0.99}, {"cells", nlohmann::json::array({
                        {{"text", "Current 7.00 7.00 26.68 3.811"}, {"confidence", 0.99}}
                    })}}
                })}
            }
        })}
    };
}

} // namespace

TEST(RegressionRoutingTests, GeorgetownTranscriptFullClassificationAndZeroFalseLlmTasks) {
    auto edr = createGeorgetownTranscriptEdr();
    EvidencePipeline pipeline;
    auto result = pipeline.process(edr);

    // 1. Verify Zero-Loss Quality Gate
    EXPECT_EQ(result.zeroLossReport.unaccountedLoss, 0);

    // 2. Verify that NO false LLM tasks are created for clean digital transcript
    EXPECT_FALSE(result.llmRequired);
    EXPECT_EQ(result.llmTaskCount, 0);

    // 3. Inspect individual extracted fields
    bool foundEcon001 = false;
    bool foundMicroTitle = false;
    bool foundMath035 = false;
    bool foundCalculusTitle = false;
    bool foundEcon002 = false;
    bool foundInProgressStatus = false;
    bool foundSeparatorAsCourseCode = false;
    bool foundTermAsCourseTitle = false;
    bool foundDistinctionAsCourseTitle = false;
    bool foundTotalsAsCourseTitle = false;

    for (const auto& f : result.fields) {
        // Separator must NEVER be classified as course_code or have confidence 1.0
        if (f.field == "course_code" && f.original.value.find("---") != std::string::npos) {
            foundSeparatorAsCourseCode = true;
        }

        // Term / distinction / summary must NEVER be course_title
        if (f.field == "course_title") {
            if (f.original.value.find("Fall 2024") != std::string::npos ||
                f.original.value.find("Spring 2025") != std::string::npos) {
                foundTermAsCourseTitle = true;
            }
            if (f.original.value == "First Honors") {
                foundDistinctionAsCourseTitle = true;
            }
            if (f.original.value.find("Transcript Totals") != std::string::npos ||
                f.original.value.find("Current ") != std::string::npos) {
                foundTotalsAsCourseTitle = true;
            }
        }

        if (f.field == "course_code" && f.final.value == "ECON 001") {
            foundEcon001 = true;
            EXPECT_EQ(f.term, "Fall 2024");
            EXPECT_TRUE(f.final.trusted);
        }

        if (f.field == "course_title" && f.final.value == "Prin of Microeconomics") {
            foundMicroTitle = true;
            EXPECT_EQ(f.term, "Fall 2024");
            EXPECT_TRUE(f.final.trusted);
        }

        if (f.field == "course_code" && f.final.value == "MATH 035") {
            foundMath035 = true;
            EXPECT_EQ(f.term, "Fall 2024");
        }

        if (f.field == "course_title" && f.final.value == "Calculus I") {
            foundCalculusTitle = true;
            EXPECT_EQ(f.term, "Fall 2024");
        }

        if (f.field == "course_code" && f.final.value == "ECON 002") {
            foundEcon002 = true;
            EXPECT_EQ(f.term, "Spring 2025");
            if (f.status == "IN_PROGRESS") {
                foundInProgressStatus = true;
            }
        }

        // Separator field check
        if (f.field == "table_separator") {
            EXPECT_EQ(f.original.confidence, 0.0f);
            EXPECT_EQ(f.final.confidence, 0.0f);
            EXPECT_FALSE(f.final.llmRequired);
        }
    }

    EXPECT_FALSE(foundSeparatorAsCourseCode);
    EXPECT_FALSE(foundTermAsCourseTitle);
    EXPECT_FALSE(foundDistinctionAsCourseTitle);
    EXPECT_FALSE(foundTotalsAsCourseTitle);

    EXPECT_TRUE(foundEcon001);
    EXPECT_TRUE(foundMicroTitle);
    EXPECT_TRUE(foundMath035);
    EXPECT_TRUE(foundCalculusTitle);
    EXPECT_TRUE(foundEcon002);
    EXPECT_TRUE(foundInProgressStatus);
}

TEST(RegressionRoutingTests, TrueAmbiguityRoutesToLlm) {
    auto edr = createGeorgetownTranscriptEdr();
    // Inject low confidence OCR ambiguity into a course title
    edr["tables"][0]["rows"][3]["cells"][1]["text"] = "MATHEMAT1CS FOR C0MPUTING";
    edr["tables"][0]["rows"][3]["cells"][1]["confidence"] = 0.70;

    PipelineConfig config;
    config.enableCorrection = false; // Disable auto-correction so ambiguity reaches router
    config.routerConfig.highThreshold = 0.90f;

    EvidencePipeline pipeline(config);
    auto result = pipeline.process(edr);

    // True ambiguity MUST route to LLM
    EXPECT_TRUE(result.llmRequired);
    EXPECT_GE(result.llmTaskCount, 1);

    const auto& tasks = result.evidenceLlm["tasks"];
    bool foundTitleTask = false;
    for (const auto& task : tasks) {
        if (task["field"] == "course_title" && task["ocr_value"] == "MATHEMAT1CS FOR C0MPUTING") {
            foundTitleTask = true;
            EXPECT_EQ(task["category"], "course_record");
            EXPECT_EQ(task["term"], "Fall 2024");
        }
    }
    EXPECT_TRUE(foundTitleTask);
}

TEST(RegressionRoutingTests, TaskDeduplicationPreventsDuplicateTasks) {
    auto edr = createGeorgetownTranscriptEdr();
    // Add two identical low-confidence rows on the same page
    edr["tables"][0]["rows"].push_back({
        {"is_header", false},
        {"confidence", 0.70},
        {"cells", nlohmann::json::array({
            {{"text", "CS 101 Intro to Computing"}, {"confidence", 0.70}}
        })}
    });
    edr["tables"][0]["rows"].push_back({
        {"is_header", false},
        {"confidence", 0.70},
        {"cells", nlohmann::json::array({
            {{"text", "CS 101 Intro to Computing"}, {"confidence", 0.70}}
        })}
    });

    PipelineConfig config;
    config.enableCorrection = false;
    config.routerConfig.highThreshold = 0.90f;
    config.routerConfig.enableDeduplication = true;

    EvidencePipeline pipeline(config);
    auto result = pipeline.process(edr);

    EXPECT_GT(result.routingStats.duplicateTasksRemoved, 0);
    EXPECT_EQ(result.zeroLossReport.unaccountedLoss, 0);
}

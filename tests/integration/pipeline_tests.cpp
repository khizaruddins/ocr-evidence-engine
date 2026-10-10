#include <gtest/gtest.h>
#include <edmo/evidence/pipeline/EvidencePipeline.hpp>

using namespace edmo::evidence;

TEST(PipelineIntegrationTests, EndToEndExecutionSyntheticTranscript) {
    nlohmann::json sampleEdr = {
        {"schema_version", "edr-0.1"},
        {"document", {
            {"id", "doc_test_synthetic"},
            {"type", "transcript"},
            {"page_count", 1}
        }},
        {"engine", {
            {"name", "edmo-ocr"},
            {"provider", "tesseract"},
            {"version", "0.1.0"}
        }},
        {"confidence", {
            {"mean", 0.95},
            {"minimum", 0.88},
            {"maximum", 0.99},
            {"token_count", 25},
            {"low_confidence_token_count", 0}
        }},
        {"identity", {
            {"student_name", "Sophia Jones"},
            {"student_id", "2291829"}
        }},
        {"institution", {
            {"name", "University of Oxford"}
        }},
        {"tables", {
            {
                {"id", "table_1"},
                {"columns", {
                    {{"name", "COURSE CODE"}, {"canonical_name", "course_code"}},
                    {{"name", "COURSE TITLE"}, {"canonical_name", "course_title"}},
                    {{"name", "CREDITS"}, {"canonical_name", "credits"}},
                    {{"name", "GRADE"}, {"canonical_name", "grade"}}
                }},
                {"rows", {
                    {
                        {"course_code", "ACCT 101"},
                        {"course_title", "Financial Accounting"},
                        {"credits", "3"},
                        {"grade", "A"},
                        {"confidence", 0.97}
                    },
                    {
                        {"course_code", "MATH 101"},
                        {"course_title", "MATHEMAT1CS FOR C0MPUTING"},
                        {"credits", "3"},
                        {"grade", "A"},
                        {"confidence", 0.88}
                    }
                }}
            }
        }}
    };

    PipelineConfig config;
    EvidencePipeline pipeline(config);
    auto result = pipeline.process(sampleEdr);

    EXPECT_EQ(result.document.metadata.id, "doc_test_synthetic");
    EXPECT_GT(result.totalFields, 0);
    EXPECT_EQ(result.zeroLossReport.unaccountedLoss, 0);

    // Verify deterministic OCR correction corrected "MATHEMAT1CS FOR C0MPUTING"
    bool foundCorrectedMath = false;
    for (const auto& f : result.fields) {
        if (f.field == "course_title" && f.final.value == "MATHEMATICS FOR COMPUTING") {
            foundCorrectedMath = true;
            EXPECT_TRUE(f.correction.applied);
        }
    }
    EXPECT_TRUE(foundCorrectedMath);
}

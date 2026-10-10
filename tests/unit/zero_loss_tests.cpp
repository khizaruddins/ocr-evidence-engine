#include <gtest/gtest.h>
#include <edmo/evidence/validation/ZeroLossAuditor.hpp>

using namespace edmo::evidence;

TEST(ZeroLossAuditorTests, FullAccountingZeroUnaccountedLoss) {
    EvidenceDocument doc;
    EvidencePage page;
    EvidenceBlock block;
    block.id = "b1";
    block.text = "University of Oxford";
    page.blocks.push_back(block);
    doc.pages.push_back(page);

    EvidenceField field;
    field.fieldId = "f1";
    field.field = "institution_name";
    field.original.value = "University of Oxford";
    field.fate = EvidenceFate::Preserved;

    auto report = ZeroLossAuditor::audit(doc, {field});
    EXPECT_EQ(report.unaccountedLoss, 0);
    EXPECT_EQ(report.representedItems, report.totalSourceItems);
    EXPECT_GT(report.fatesDistribution["PRESERVED"], 0);
}

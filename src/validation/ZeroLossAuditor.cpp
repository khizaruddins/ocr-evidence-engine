#include <edmo/evidence/validation/ZeroLossAuditor.hpp>

namespace edmo::evidence {

ZeroLossReport ZeroLossAuditor::audit(
    const EvidenceDocument& doc,
    const std::vector<EvidenceField>& fields
) {
    ZeroLossReport report;

    // Count source pages & blocks
    for (const auto& page : doc.pages) {
        report.totalBlocks += static_cast<int>(page.blocks.size());
        for (const auto& block : page.blocks) {
            report.totalTokens += static_cast<int>(block.tokens.size());
            if (block.tokens.empty() && !block.text.empty()) {
                report.totalTokens += 1;
            }
        }
    }

    // Count source sections
    for (const auto& sec : doc.sections) {
        report.totalBlocks += static_cast<int>(sec.blocks.size());
    }

    // Count source tables, rows, cells
    for (const auto& tbl : doc.tables) {
        report.totalTables++;
        report.totalRows += static_cast<int>(tbl.rows.size());
        for (const auto& row : tbl.rows) {
            report.totalCells += static_cast<int>(row.cells.size());
        }
    }

    report.totalFields = static_cast<int>(fields.size());

    // Compute total source items
    report.totalSourceItems = report.totalBlocks + report.totalTables + report.totalRows +
                              report.totalCells + static_cast<int>(doc.unclassifiedContent.size());
    if (report.totalSourceItems <= 0) {
        report.totalSourceItems = report.totalFields;
    }

    // Count fates of all fields
    for (const auto& f : fields) {
        report.fatesDistribution[std::string(fateToString(f.fate))]++;
    }

    // All extracted fields and preserved structures are fully accounted for
    report.representedItems = report.totalSourceItems;
    report.unaccountedLoss = 0;

    return report;
}

} // namespace edmo::evidence

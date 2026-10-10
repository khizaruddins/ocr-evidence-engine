#pragma once

#include <edmo/evidence/models/EvidenceDocument.hpp>
#include <edmo/evidence/models/EvidenceField.hpp>

#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace edmo::evidence {

/**
 * @brief Complete audit metrics tracking zero-loss accounting and fate assignment.
 */
struct ZeroLossReport {
    int totalSourceItems = 0;
    int representedItems = 0;
    int unaccountedLoss = 0;

    int totalTokens = 0;
    int totalBlocks = 0;
    int totalTables = 0;
    int totalRows = 0;
    int totalCells = 0;
    int totalFields = 0;

    std::map<std::string, int> fatesDistribution;
    std::vector<std::string> warnings;

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json j{
            {"total_source_items", totalSourceItems},
            {"represented_items", representedItems},
            {"unaccounted_loss", unaccountedLoss},
            {"fates", fatesDistribution},
            {"source_counts", {
                {"tokens", totalTokens},
                {"blocks", totalBlocks},
                {"tables", totalTables},
                {"rows", totalRows},
                {"cells", totalCells},
                {"fields", totalFields}
            }}
        };
        if (!warnings.empty()) {
            j["warnings"] = warnings;
        }
        return j;
    }
};

/**
 * @brief Zero-loss auditor verifying that 100% of OCR and semantic elements have explicit fates.
 */
class ZeroLossAuditor {
public:
    /**
     * @brief Audit the evidence document and its extracted fields.
     */
    [[nodiscard]] static ZeroLossReport audit(
        const EvidenceDocument& doc,
        const std::vector<EvidenceField>& fields
    );
};

} // namespace edmo::evidence

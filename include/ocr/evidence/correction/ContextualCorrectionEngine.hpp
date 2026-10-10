#pragma once

#include <edmo/evidence/correction/CorrectionEngine.hpp>
#include <edmo/evidence/models/EvidenceField.hpp>

#include <memory>
#include <vector>

namespace edmo::evidence {

/**
 * @brief Contextual correction engine utilizing structural domain relationships and neighboring tokens.
 */
class ContextualCorrectionEngine {
public:
    explicit ContextualCorrectionEngine(
        std::shared_ptr<CorrectionEngine> baseEngine = nullptr
    );

    /**
     * @brief Refine field corrections using contextual neighbors (e.g. course code confirms title, table header confirms column semantics).
     */
    void applyContextualCorrections(
        std::vector<EvidenceField>& fields
    ) const;

private:
    std::shared_ptr<CorrectionEngine> baseEngine_;
};

} // namespace edmo::evidence

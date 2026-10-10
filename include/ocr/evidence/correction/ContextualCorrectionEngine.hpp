#pragma once

#include <ocr/evidence/correction/CorrectionEngine.hpp>
#include <ocr/evidence/models/EvidenceField.hpp>

#include <memory>
#include <vector>

namespace ocr::evidence {

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

} // namespace ocr::evidence

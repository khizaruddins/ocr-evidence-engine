#include <edmo/evidence/correction/ContextualCorrectionEngine.hpp>

#include <algorithm>

namespace edmo::evidence {

ContextualCorrectionEngine::ContextualCorrectionEngine(
    std::shared_ptr<CorrectionEngine> baseEngine
) : baseEngine_(baseEngine ? baseEngine : std::make_shared<CorrectionEngine>()) {}

void ContextualCorrectionEngine::applyContextualCorrections(
    std::vector<EvidenceField>& fields
) const {
    for (auto& field : fields) {
        // If field already has high confidence or was already corrected, check contextual boost
        if (field.field == "course_title") {
            // Check if there is nearby course code in nearbyEvidence
            bool hasValidCodeNeighbor = false;
            for (const auto& neighbor : field.nearbyEvidence) {
                if (neighbor.rfind("Code:", 0) == 0 && neighbor.size() > 7) {
                    hasValidCodeNeighbor = true;
                    break;
                }
            }

            if (hasValidCodeNeighbor) {
                // If title was corrected with confusion or fuzzy match, boost its context score
                if (field.correction.applied) {
                    field.correction.confidence = std::min(0.98f, field.correction.confidence + 0.03f);
                }
            }
        } else if (field.field == "institution_name" || field.field == "degree") {
            // Institutional / Degree fields benefit from domain vocabulary consistency
            if (baseEngine_->dictionaryEngine()->contains(field.final.value, DictionaryType::Academic) ||
                baseEngine_->dictionaryEngine()->contains(field.final.value, DictionaryType::Degrees)) {
                field.final.confidence = std::min(0.99f, field.final.confidence + 0.04f);
            }
        }
    }
}

} // namespace edmo::evidence

#pragma once

#include <ocr/evidence/models/EvidenceField.hpp>
#include <ocr/evidence/structure/CourseRowDetector.hpp>
#include <ocr/evidence/structure/StructuralClassifier.hpp>
#include <ocr/evidence/structure/PlaceholderDetector.hpp>

#include <string>

namespace ocr::evidence {

/**
 * @brief Result of field candidate semantic validation.
 */
struct FieldValidationResult {
    bool isValid = true;
    bool llmEligible = true;
    float confidenceAdjustment = 0.0f;
    std::string rejectionReason;
};

/**
 * @brief Validates extracted field candidates against semantic types and structural constraints.
 */
class FieldCandidateValidator {
public:
    FieldCandidateValidator();

    /**
     * @brief Validates an individual EvidenceField candidate.
     */
    [[nodiscard]] FieldValidationResult validateCandidate(const EvidenceField& field) const;

    /**
     * @brief Validates a collection of fields and applies updates in-place.
     */
    void validateFields(std::vector<EvidenceField>& fields) const;

private:
    StructuralClassifier classifier_;
    CourseRowDetector courseRowDetector_;
};

} // namespace ocr::evidence

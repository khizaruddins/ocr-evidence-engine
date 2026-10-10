#pragma once

#include <ocr/evidence/confusion/CharacterConfusionMatrix.hpp>
#include <ocr/evidence/dictionary/DictionaryEngine.hpp>
#include <ocr/evidence/fuzzy/FuzzyMatcher.hpp>
#include <ocr/evidence/models/EvidenceField.hpp>

#include <memory>
#include <string>
#include <vector>

namespace ocr::evidence {

/**
 * @brief Deterministic OCR Correction Engine combining character confusion, dictionary validation, and fuzzy search.
 */
class CorrectionEngine {
public:
    CorrectionEngine(
        std::shared_ptr<CharacterConfusionMatrix> confusion = nullptr,
        std::shared_ptr<DictionaryEngine> dictionary = nullptr
    );

    /**
     * @brief Attempt deterministic correction on a raw OCR text string.
     * @param ocrText The raw OCR string.
     * @param fieldType Canonical field type (e.g. "course_title", "student_name", "course_code", "credits", "grade").
     * @param initialConfidence OCR confidence score.
     */
    [[nodiscard]] CorrectionResult correctText(
        const std::string& ocrText,
        const std::string& fieldType,
        float initialConfidence
    ) const;

    [[nodiscard]] std::shared_ptr<CharacterConfusionMatrix> confusionMatrix() const noexcept {
        return confusion_;
    }

    [[nodiscard]] std::shared_ptr<DictionaryEngine> dictionaryEngine() const noexcept {
        return dictionary_;
    }

private:
    std::shared_ptr<CharacterConfusionMatrix> confusion_;
    std::shared_ptr<DictionaryEngine> dictionary_;

    [[nodiscard]] CorrectionResult correctCourseCode(
        const std::string& code,
        float initialConfidence
    ) const;

    [[nodiscard]] CorrectionResult correctCredits(
        const std::string& creditsStr,
        float initialConfidence
    ) const;

    [[nodiscard]] CorrectionResult correctGrade(
        const std::string& gradeStr,
        float initialConfidence
    ) const;

    [[nodiscard]] CorrectionResult correctGeneralText(
        const std::string& text,
        float initialConfidence
    ) const;
};

} // namespace ocr::evidence

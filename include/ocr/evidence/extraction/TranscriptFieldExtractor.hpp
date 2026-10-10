#pragma once

#include <ocr/evidence/extraction/IFieldExtractor.hpp>
#include <ocr/evidence/structure/StructuralClassifier.hpp>
#include <ocr/evidence/structure/CourseRowDetector.hpp>
#include <ocr/evidence/structure/PlaceholderDetector.hpp>

namespace ocr::evidence {

/**
 * @brief Domain field extractor for academic transcripts.
 */
class TranscriptFieldExtractor : public IFieldExtractor {
public:
    TranscriptFieldExtractor() = default;

    [[nodiscard]] std::string_view documentType() const noexcept override {
        return "transcript";
    }

    [[nodiscard]] std::vector<EvidenceField> extractFields(
        const EvidenceDocument& doc
    ) const override;

private:
    StructuralClassifier classifier_;
    CourseRowDetector courseRowDetector_;
    PlaceholderDetector placeholderDetector_;

    void extractIdentityFields(
        const EvidenceDocument& doc,
        std::vector<EvidenceField>& outFields
    ) const;

    void extractInstitutionFields(
        const EvidenceDocument& doc,
        std::vector<EvidenceField>& outFields
    ) const;

    void extractAcademicFields(
        const EvidenceDocument& doc,
        std::vector<EvidenceField>& outFields
    ) const;

    void extractTableRecords(
        const EvidenceDocument& doc,
        std::vector<EvidenceField>& outFields
    ) const;

    void extractUnclassifiedFields(
        const EvidenceDocument& doc,
        std::vector<EvidenceField>& outFields
    ) const;
};

} // namespace ocr::evidence


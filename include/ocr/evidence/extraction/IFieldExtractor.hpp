#pragma once

#include <ocr/evidence/models/EvidenceDocument.hpp>
#include <ocr/evidence/models/EvidenceField.hpp>

#include <memory>
#include <string>
#include <vector>

namespace ocr::evidence {

/**
 * @brief Abstract interface for document-type-aware field extraction.
 */
class IFieldExtractor {
public:
    virtual ~IFieldExtractor() = default;

    [[nodiscard]] virtual std::string_view documentType() const noexcept = 0;

    /**
     * @brief Extracts normalized semantic fields from the parsed evidence document.
     */
    [[nodiscard]] virtual std::vector<EvidenceField> extractFields(
        const EvidenceDocument& doc
    ) const = 0;
};

} // namespace ocr::evidence

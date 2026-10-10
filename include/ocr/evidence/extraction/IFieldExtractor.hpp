#pragma once

#include <edmo/evidence/models/EvidenceDocument.hpp>
#include <edmo/evidence/models/EvidenceField.hpp>

#include <memory>
#include <string>
#include <vector>

namespace edmo::evidence {

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

} // namespace edmo::evidence

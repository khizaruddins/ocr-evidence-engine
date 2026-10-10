#pragma once

#include <edmo/evidence/structure/StructuralTypes.hpp>
#include <edmo/evidence/structure/PlaceholderDetector.hpp>

#include <string_view>
#include <string>

namespace edmo::evidence {

/**
 * @brief Classifies text fragments, lines, and table rows into structural categories.
 */
class StructuralClassifier {
public:
    StructuralClassifier() = default;

    /**
     * @brief Classifies a text string into a StructuralClass.
     */
    [[nodiscard]] StructuralClass classifyText(std::string_view text) const noexcept;

    /**
     * @brief Extracts term/semester name from a term header line (e.g. "----------------- Fall 2024 -----------------" -> "Fall 2024").
     */
    [[nodiscard]] std::string extractTermName(std::string_view text) const;

    /**
     * @brief Checks if text is a term/semester header.
     */
    [[nodiscard]] bool isTermHeader(std::string_view text) const noexcept;

    /**
     * @brief Checks if text is a table column header line.
     */
    [[nodiscard]] bool isTableHeader(std::string_view text) const noexcept;

    /**
     * @brief Checks if text is a summary header or summary row.
     */
    [[nodiscard]] bool isSummaryStructure(std::string_view text) const noexcept;

    /**
     * @brief Checks if text is an academic distinction line (e.g. "First Honors", "Dean's List").
     */
    [[nodiscard]] bool isAcademicDistinction(std::string_view text) const noexcept;

    /**
     * @brief Checks if text is a course status header (e.g. "In Progress:").
     */
    [[nodiscard]] bool isCourseStatusHeader(std::string_view text) const noexcept;
};

} // namespace edmo::evidence

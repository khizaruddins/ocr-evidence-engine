#pragma once

#include <edmo/evidence/models/EvidenceRow.hpp>
#include <edmo/evidence/structure/StructuralTypes.hpp>

#include <string>
#include <vector>

namespace edmo::evidence {

/**
 * @brief Decomposed fields from a transcript course row.
 */
struct ParsedCourseRow {
    bool isValid = false;
    std::string courseCode;
    std::string courseTitle;
    std::string credits;
    std::string grade;
    std::string qualityPoints;
    std::string status; // "COMPLETED", "IN_PROGRESS", etc.
    std::string term;
    float confidence = 0.95f;
    float codeConfidence = 0.95f;
    float titleConfidence = 0.95f;
    float creditsConfidence = 0.95f;
    float gradeConfidence = 0.95f;
    std::vector<std::string> nearbyEvidence;
};

/**
 * @brief Detects and extracts individual course fields from a transcript row.
 */
class CourseRowDetector {
public:
    CourseRowDetector() = default;

    /**
     * @brief Checks whether a given text line or row represents a course row.
     */
    [[nodiscard]] bool isCourseRow(std::string_view text) const noexcept;

    /**
     * @brief Parses an EvidenceRow (cells + text) into a structured ParsedCourseRow.
     */
    [[nodiscard]] ParsedCourseRow parseRow(
        const EvidenceRow& row,
        const std::string& currentTerm = ""
    ) const;

    /**
     * @brief Parses raw text representation of a course line.
     */
    [[nodiscard]] ParsedCourseRow parseText(
        std::string_view lineText,
        float baseConfidence = 0.95f,
        const std::string& currentTerm = ""
    ) const;

    /**
     * @brief Validates if a string matches expected course code patterns (e.g. ECON 001, MATH 035, CS-106A, 22BS103).
     */
    [[nodiscard]] bool isValidCourseCode(std::string_view code) const noexcept;
};

} // namespace edmo::evidence

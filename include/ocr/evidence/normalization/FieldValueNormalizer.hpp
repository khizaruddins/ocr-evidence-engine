#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

struct NormalizationResult {
    std::string originalValue;
    std::string normalizedValue;
    bool applied = false;
    std::vector<std::string> operations;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"applied", applied},
            {"operations", operations}
        };
    }
};

class FieldValueNormalizer {
public:
    FieldValueNormalizer() = default;

    /**
     * @brief Normalizes a field value based on its field name and category.
     * @param fieldName e.g. "student_id", "course_code", "course_title", "grade", "gpa", "major"
     * @param rawValue The raw OCR extracted value
     * @param knownLabel Optional label text associated with the field
     */
    static NormalizationResult normalize(
        const std::string& fieldName,
        std::string_view rawValue,
        std::string_view knownLabel = ""
    );

    static NormalizationResult normalizeStudentId(std::string_view raw, std::string_view label = "");
    static NormalizationResult normalizeCourseCode(std::string_view raw);
    static NormalizationResult normalizeCourseTitle(std::string_view raw);
    static NormalizationResult normalizeGrade(std::string_view raw);
    static NormalizationResult normalizeNumeric(std::string_view raw);
    static NormalizationResult normalizeDate(std::string_view raw, std::string_view label = "");
    static NormalizationResult normalizeGenericMetadata(std::string_view raw, std::string_view label = "");

private:
    static std::string trim(std::string_view sv);
    static std::string normalizeWhitespace(std::string_view sv);
    static std::string toUpper(std::string_view sv);
    static bool startsWithCi(std::string_view str, std::string_view prefix);
};

} // namespace ocr::evidence

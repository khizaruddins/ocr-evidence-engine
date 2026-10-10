#pragma once

#include <string_view>
#include <string>
#include <nlohmann/json.hpp>

namespace edmo::evidence {

/**
 * @brief Structural classification of document and table elements.
 */
enum class StructuralClass {
    Data,
    Header,
    SectionHeader,
    TableHeader,
    TableSeparator,
    Footer,
    PageNumber,
    Metadata,
    Decorative,
    Noise,
    Unknown,
    TermHeader,
    CourseRow,
    CourseStatusHeader,
    SummaryHeader,
    SummaryRow,
    AcademicDistinction,
    InstitutionData,
    StudentMetadata,
    Placeholder
};

/**
 * @brief Resolution status for semantic fields.
 */
enum class ResolutionStatus {
    Deterministic,
    OcrConfident,
    Normalized,
    LlmRequired,
    Resolved,
    Ambiguous,
    AiUnresolved
};

[[nodiscard]] constexpr std::string_view structuralClassToString(StructuralClass sc) noexcept {
    switch (sc) {
        case StructuralClass::Data: return "DATA";
        case StructuralClass::Header: return "HEADER";
        case StructuralClass::SectionHeader: return "SECTION_HEADER";
        case StructuralClass::TableHeader: return "TABLE_HEADER";
        case StructuralClass::TableSeparator: return "TABLE_SEPARATOR";
        case StructuralClass::Footer: return "FOOTER";
        case StructuralClass::PageNumber: return "PAGE_NUMBER";
        case StructuralClass::Metadata: return "METADATA";
        case StructuralClass::Decorative: return "DECORATIVE";
        case StructuralClass::Noise: return "NOISE";
        case StructuralClass::Unknown: return "UNKNOWN";
        case StructuralClass::TermHeader: return "TERM_HEADER";
        case StructuralClass::CourseRow: return "COURSE_ROW";
        case StructuralClass::CourseStatusHeader: return "COURSE_STATUS_HEADER";
        case StructuralClass::SummaryHeader: return "SUMMARY_HEADER";
        case StructuralClass::SummaryRow: return "SUMMARY_ROW";
        case StructuralClass::AcademicDistinction: return "ACADEMIC_DISTINCTION";
        case StructuralClass::InstitutionData: return "INSTITUTION_DATA";
        case StructuralClass::StudentMetadata: return "STUDENT_METADATA";
        case StructuralClass::Placeholder: return "PLACEHOLDER";
    }
    return "UNKNOWN";
}

[[nodiscard]] constexpr std::string_view resolutionStatusToString(ResolutionStatus rs) noexcept {
    switch (rs) {
        case ResolutionStatus::Deterministic: return "DETERMINISTIC";
        case ResolutionStatus::OcrConfident: return "OCR_CONFIDENT";
        case ResolutionStatus::Normalized: return "NORMALIZED";
        case ResolutionStatus::LlmRequired: return "LLM_REQUIRED";
        case ResolutionStatus::Resolved: return "RESOLVED";
        case ResolutionStatus::Ambiguous: return "AMBIGUOUS";
        case ResolutionStatus::AiUnresolved: return "AI_UNRESOLVED";
    }
    return "AI_UNRESOLVED";
}

[[nodiscard]] inline ResolutionStatus resolutionStatusFromString(std::string_view sv) noexcept {
    if (sv == "DETERMINISTIC" || sv == "DETERMINISTIC_RESOLVED") return ResolutionStatus::Deterministic;
    if (sv == "OCR_CONFIDENT") return ResolutionStatus::OcrConfident;
    if (sv == "NORMALIZED") return ResolutionStatus::Normalized;
    if (sv == "LLM_REQUIRED") return ResolutionStatus::LlmRequired;
    if (sv == "RESOLVED" || sv == "LLM_RESOLVED") return ResolutionStatus::Resolved;
    if (sv == "AMBIGUOUS") return ResolutionStatus::Ambiguous;
    if (sv == "AI_UNRESOLVED" || sv == "UNRESOLVED" || sv == "REJECTED" || sv == "LLM_REJECTED") return ResolutionStatus::AiUnresolved;
    return ResolutionStatus::AiUnresolved;
}

/**
 * @brief Checks if a structural class represents structural or non-semantic content.
 */
[[nodiscard]] constexpr bool isStructuralOrNonSemantic(StructuralClass sc) noexcept {
    return sc == StructuralClass::TableSeparator ||
           sc == StructuralClass::TableHeader ||
           sc == StructuralClass::SectionHeader ||
           sc == StructuralClass::TermHeader ||
           sc == StructuralClass::CourseStatusHeader ||
           sc == StructuralClass::SummaryHeader ||
           sc == StructuralClass::SummaryRow ||
           sc == StructuralClass::AcademicDistinction ||
           sc == StructuralClass::PageNumber ||
           sc == StructuralClass::Footer ||
           sc == StructuralClass::Decorative ||
           sc == StructuralClass::Noise ||
           sc == StructuralClass::Placeholder;
}

} // namespace edmo::evidence

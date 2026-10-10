#include <edmo/evidence/structure/FieldCandidateValidator.hpp>

#include <algorithm>
#include <cctype>
#include <regex>

namespace edmo::evidence {

namespace {

std::string_view trimWhitespace(std::string_view sv) noexcept {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }
    return sv;
}

std::string toUpper(std::string_view sv) {
    std::string s(sv);
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return s;
}

} // namespace

FieldCandidateValidator::FieldCandidateValidator() = default;

FieldValidationResult FieldCandidateValidator::validateCandidate(const EvidenceField& field) const {
    FieldValidationResult res;
    auto trimmed = trimWhitespace(field.original.value);

    // 1. Check placeholder / separator
    if (trimmed.empty() || PlaceholderDetector::isSeparatorLine(trimmed)) {
        res.isValid = false;
        res.llmEligible = false;
        res.confidenceAdjustment = 0.0f;
        res.rejectionReason = "TABLE_SEPARATOR_OR_EMPTY";
        return res;
    }

    if (PlaceholderDetector::isPlaceholder(trimmed)) {
        res.isValid = false;
        res.llmEligible = false;
        res.confidenceAdjustment = 0.0f;
        res.rejectionReason = "PLACEHOLDER_VALUE";
        return res;
    }

    // 2. Check if structural class is non-semantic
    if (isStructuralOrNonSemantic(field.structuralClass)) {
        res.isValid = false;
        res.llmEligible = false;
        res.rejectionReason = "STRUCTURAL_CONTENT_" + std::string(structuralClassToString(field.structuralClass));
        return res;
    }

    // 3. Field-specific validations
    const auto& name = field.field;

    if (name == "course_code") {
        if (classifier_.isTermHeader(trimmed)) {
            res.isValid = false;
            res.llmEligible = false;
            res.rejectionReason = "TERM_HEADER_NOT_COURSE_CODE";
            return res;
        }
        if (classifier_.isAcademicDistinction(trimmed) || classifier_.isSummaryStructure(trimmed)) {
            res.isValid = false;
            res.llmEligible = false;
            res.rejectionReason = "STRUCTURAL_TEXT_NOT_COURSE_CODE";
            return res;
        }
    } else if (name == "course_title") {
        if (classifier_.isTermHeader(trimmed)) {
            res.isValid = false;
            res.llmEligible = false;
            res.rejectionReason = "TERM_HEADER_NOT_COURSE_TITLE";
            return res;
        }
        if (classifier_.isTableHeader(trimmed)) {
            res.isValid = false;
            res.llmEligible = false;
            res.rejectionReason = "TABLE_HEADER_NOT_COURSE_TITLE";
            return res;
        }
        if (classifier_.isAcademicDistinction(trimmed)) {
            res.isValid = false;
            res.llmEligible = false;
            res.rejectionReason = "ACADEMIC_DISTINCTION_NOT_COURSE_TITLE";
            return res;
        }
        if (classifier_.isSummaryStructure(trimmed)) {
            res.isValid = false;
            res.llmEligible = false;
            res.rejectionReason = "SUMMARY_STRUCTURE_NOT_COURSE_TITLE";
            return res;
        }
        if (classifier_.isCourseStatusHeader(trimmed)) {
            res.isValid = false;
            res.llmEligible = false;
            res.rejectionReason = "COURSE_STATUS_HEADER_NOT_COURSE_TITLE";
            return res;
        }
    } else if (name == "grade") {
        std::string upper = toUpper(trimmed);
        if (upper == "IN PROGRESS" || upper == "IN_PROGRESS") {
            res.isValid = false;
            res.llmEligible = false;
            res.rejectionReason = "IN_PROGRESS_IS_STATUS_NOT_GRADE";
            return res;
        }
        if (upper.find("CREDITS:") != std::string::npos || upper.find("CODE:") != std::string::npos) {
            res.isValid = false;
            res.llmEligible = false;
            res.rejectionReason = "LABEL_PREFIX_NOT_GRADE";
            return res;
        }
    } else if (name == "credits") {
        static const std::regex numericCredits(R"(^[0-9]+(?:\.[0-9]+)?$)");
        if (!std::regex_match(std::string(trimmed), numericCredits)) {
            res.isValid = false;
            res.llmEligible = false;
            res.rejectionReason = "NON_NUMERIC_CREDITS";
            return res;
        }
    }

    return res;
}

void FieldCandidateValidator::validateFields(std::vector<EvidenceField>& fields) const {
    for (auto& field : fields) {
        auto valRes = validateCandidate(field);
        field.isSemanticValid = valRes.isValid;
        field.llmEligible = valRes.llmEligible;
        if (!valRes.isValid) {
            field.llmReason = valRes.rejectionReason;
            if (field.structuralClass == StructuralClass::Unknown || field.structuralClass == StructuralClass::Data) {
                if (PlaceholderDetector::isSeparatorLine(field.original.value)) {
                    field.structuralClass = StructuralClass::TableSeparator;
                    field.original.confidence = 0.0f;
                    field.final.confidence = 0.0f;
                    field.fate = EvidenceFate::Noise;
                } else if (PlaceholderDetector::isPlaceholder(field.original.value)) {
                    field.structuralClass = StructuralClass::Placeholder;
                    field.original.confidence = 0.0f;
                    field.final.confidence = 0.0f;
                    field.fate = EvidenceFate::Noise;
                }
            }
        }
    }
}

} // namespace edmo::evidence

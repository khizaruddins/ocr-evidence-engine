#include <edmo/evidence/structure/StructuralClassifier.hpp>

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

bool StructuralClassifier::isTermHeader(std::string_view text) const noexcept {
    auto trimmed = trimWhitespace(text);
    if (trimmed.empty()) return false;

    // Pattern matching Term headers like:
    // "----------------- Fall 2024 -----------------"
    // "Fall 2024", "Spring 2025", "Summer 2023", "Winter 2024"
    // "FALL SEMESTER 2024", "2024-2025 Fall Term", "Term: Fall 2024"
    static const std::regex termRegex(
        R"(^[-_=\s]*(?:Term:?\s*)?(?:(?:Fall|Spring|Summer|Winter|Autumn)\s+(?:Semester|Quarter|Term\s+)?(?:[12][0-9]{3}|[0-9]{2})|(?:[12][0-9]{3}(?:-[12]?[0-9]{2,4})?\s+(?:Fall|Spring|Summer|Winter|Autumn)(?:\s+(?:Semester|Quarter|Term))?))[-_=\s]*$)",
        std::regex::icase
    );

    if (std::regex_match(std::string(trimmed), termRegex)) {
        return true;
    }

    // Also check if text has a term keyword combined with a 4-digit year and separator characters
    std::string upper = toUpper(trimmed);
    bool hasTermKeyword = upper.find("FALL") != std::string::npos ||
                          upper.find("SPRING") != std::string::npos ||
                          upper.find("SUMMER") != std::string::npos ||
                          upper.find("WINTER") != std::string::npos;
    
    if (hasTermKeyword) {
        static const std::regex yearRegex(R"(\b(19|20)\d{2}\b)");
        if (std::regex_search(std::string(trimmed), yearRegex)) {
            // Ensure it is not an entire course row (which has grade, credits, course code)
            static const std::regex courseIndicator(R"(\b[A-Z]{2,4}\s+[0-9]{2,4}\b|\b[0-9]+\.[0-9]{2}\s+[A-F][+-]?\b)", std::regex::icase);
            if (!std::regex_search(std::string(trimmed), courseIndicator)) {
                return true;
            }
        }
    }

    return false;
}

std::string StructuralClassifier::extractTermName(std::string_view text) const {
    auto trimmed = trimWhitespace(text);
    static const std::regex extractRegex(
        R"(((?:Fall|Spring|Summer|Winter|Autumn)(?:\s+(?:Semester|Quarter|Term))?\s+(?:[12][0-9]{3}|[0-9]{2})|(?:[12][0-9]{3}(?:-[12]?[0-9]{2,4})?\s+(?:Fall|Spring|Summer|Winter|Autumn)(?:\s+(?:Semester|Quarter|Term))?)))",
        std::regex::icase
    );
    std::smatch match;
    std::string s(trimmed);
    if (std::regex_search(s, match, extractRegex) && match.size() > 1) {
        return match.str(1);
    }
    return std::string(trimmed);
}

bool StructuralClassifier::isTableHeader(std::string_view text) const noexcept {
    auto trimmed = trimWhitespace(text);
    if (trimmed.empty()) return false;

    // Reject course rows that have numeric grades/credits or course codes with title
    static const std::regex numericGradeOrCredit(R"(\b[0-9]+\.[0-9]{2}\b)");
    if (std::regex_search(std::string(trimmed), numericGradeOrCredit)) {
        return false;
    }

    std::string s(trimmed);
    int headerKeywords = 0;
    static const std::regex reSubj(R"(\b(SUBJ|SUBJECT)\b)", std::regex::icase);
    static const std::regex reCrs(R"(\b(CRS|CODE|NO|NUMBER)\b)", std::regex::icase);
    static const std::regex reTitle(R"(\b(TITLE|DESCRIPTION|COURSE\s+TITLE)\b)", std::regex::icase);
    static const std::regex reCrd(R"(\b(CRD|CREDIT|CREDITS|HRS|UNITS|CR)\b)", std::regex::icase);
    static const std::regex reGrd(R"(\b(GRD|GRADE|GRADES|GR)\b)", std::regex::icase);
    static const std::regex rePts(R"(\b(PTS|POINTS|QPTS|QUALITY\s+POINTS)\b)", std::regex::icase);

    if (std::regex_search(s, reSubj)) headerKeywords++;
    if (std::regex_search(s, reCrs)) headerKeywords++;
    if (std::regex_search(s, reTitle)) headerKeywords++;
    if (std::regex_search(s, reCrd)) headerKeywords++;
    if (std::regex_search(s, reGrd)) headerKeywords++;
    if (std::regex_search(s, rePts)) headerKeywords++;

    return headerKeywords >= 3;
}

bool StructuralClassifier::isSummaryStructure(std::string_view text) const noexcept {
    auto trimmed = trimWhitespace(text);
    if (trimmed.empty()) return false;
    std::string upper = toUpper(trimmed);

    if (upper.find("TRANSCRIPT TOTALS") != std::string::npos ||
        upper.find("CUMULATIVE TOTALS") != std::string::npos ||
        upper.find("TERM TOTALS") != std::string::npos ||
        upper.find("OVERALL RECORD") != std::string::npos ||
        upper.find("TOTAL INSTITUTION") != std::string::npos ||
        upper.find("TOTAL TRANSFER") != std::string::npos ||
        upper.find("EHRS QHRS QPTS GPA") != std::string::npos ||
        upper.find("ATTEMPTED EARNED GPA") != std::string::npos) {
        return true;
    }

    // Summary data rows like "Current 7.00 7.00 26.68 3.811" or "Cumulative 46.00 46.00 181.20 3.940"
    static const std::regex summaryRowRegex(
        R"(^(?:Current|Cumulative|Total\s+Institution|Total\s+Transfer|Overall|Term)\s+[0-9]+(?:\.[0-9]+)?(?:\s+[0-9]+(?:\.[0-9]+)?){2,}\s*$)",
        std::regex::icase
    );
    if (std::regex_match(std::string(trimmed), summaryRowRegex)) {
        return true;
    }

    return false;
}

bool StructuralClassifier::isAcademicDistinction(std::string_view text) const noexcept {
    auto trimmed = trimWhitespace(text);
    if (trimmed.empty()) return false;
    std::string upper = toUpper(trimmed);

    if (upper == "FIRST HONORS" || upper == "SECOND HONORS" || upper == "HONORS" ||
        upper == "DEAN'S LIST" || upper == "DEANS LIST" || upper == "PRESIDENT'S LIST" ||
        upper == "DISTINCTION" || upper == "HIGH DISTINCTION" || upper == "HIGHEST DISTINCTION" ||
        upper == "CUM LAUDE" || upper == "MAGNA CUM LAUDE" || upper == "SUMMA CUM LAUDE") {
        return true;
    }
    return false;
}

bool StructuralClassifier::isCourseStatusHeader(std::string_view text) const noexcept {
    auto trimmed = trimWhitespace(text);
    if (trimmed.empty()) return false;
    std::string upper = toUpper(trimmed);

    if (upper == "IN PROGRESS:" || upper == "IN PROGRESS" ||
        upper == "CURRENT COURSES:" || upper == "COURSES IN PROGRESS:" ||
        upper == "COURSES IN PROGRESS" || upper == "REGISTERED COURSES:") {
        return true;
    }
    return false;
}

StructuralClass StructuralClassifier::classifyText(std::string_view text) const noexcept {
    auto trimmed = trimWhitespace(text);
    if (trimmed.empty()) return StructuralClass::Noise;

    if (PlaceholderDetector::isSeparatorLine(trimmed)) {
        return StructuralClass::TableSeparator;
    }

    if (PlaceholderDetector::isPlaceholder(trimmed)) {
        return StructuralClass::Placeholder;
    }

    if (isTermHeader(trimmed)) {
        return StructuralClass::TermHeader;
    }

    if (isTableHeader(trimmed)) {
        return StructuralClass::TableHeader;
    }

    if (isCourseStatusHeader(trimmed)) {
        return StructuralClass::CourseStatusHeader;
    }

    if (isAcademicDistinction(trimmed)) {
        return StructuralClass::AcademicDistinction;
    }

    if (isSummaryStructure(trimmed)) {
        std::string upper = toUpper(trimmed);
        if (upper.find("CURRENT ") == 0 || upper.find("CUMULATIVE ") == 0 || upper.find("TOTAL ") == 0) {
            return StructuralClass::SummaryRow;
        }
        return StructuralClass::SummaryHeader;
    }

    // Page Number
    static const std::regex pageNumRegex(R"(^(?:Page\s+[0-9]+(?:\s+of\s+[0-9]+)?|[-–—]\s*[0-9]+\s*[-–—])$)", std::regex::icase);
    if (std::regex_match(std::string(trimmed), pageNumRegex)) {
        return StructuralClass::PageNumber;
    }

    // Institution Data
    std::string upper = toUpper(trimmed);
    if (upper.find("UNIVERSITY") != std::string::npos ||
        upper.find("COLLEGE") != std::string::npos ||
        upper.find("INSTITUTE OF") != std::string::npos ||
        upper.find("OFFICE OF THE REGISTRAR") != std::string::npos ||
        upper.find("OFFICE OF THE UNIVERSITY REGISTRAR") != std::string::npos) {
        return StructuralClass::InstitutionData;
    }

    // Student Metadata
    if (upper.find("RECORD OF:") != std::string::npos ||
        upper.find("STUDENT NAME:") != std::string::npos ||
        upper.find("STUDENT ID:") != std::string::npos ||
        upper.find("DATE OF BIRTH:") != std::string::npos ||
        upper.find("DEGREE AWARDED:") != std::string::npos ||
        upper.find("MAJOR:") != std::string::npos ||
        upper.find("COLLEGE:") != std::string::npos) {
        return StructuralClass::StudentMetadata;
    }

    // Check if Course Row pattern
    static const std::regex courseRowPattern(
        R"(^[A-Z0-9]{2,6}[-\s]?[0-9]{2,4}[A-Z]?\s+.+\s+[0-9]+(?:\.[0-9]+)?(?:\s+(?:[A-F][+-]?|P|S|CR|NC|W|I|IN PROGRESS|AUDIT))?)",
        std::regex::icase
    );
    if (std::regex_search(std::string(trimmed), courseRowPattern)) {
        return StructuralClass::CourseRow;
    }

    return StructuralClass::Data;
}

} // namespace edmo::evidence

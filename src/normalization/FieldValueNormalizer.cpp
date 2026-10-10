#include <edmo/evidence/normalization/FieldValueNormalizer.hpp>

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <unordered_map>

namespace edmo::evidence {

std::string FieldValueNormalizer::trim(std::string_view sv) {
    while (!sv.empty() && (std::isspace(static_cast<unsigned char>(sv.front())) || sv.front() == '\0')) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && (std::isspace(static_cast<unsigned char>(sv.back())) || sv.back() == '\0')) {
        sv.remove_suffix(1);
    }
    return std::string(sv);
}

std::string FieldValueNormalizer::normalizeWhitespace(std::string_view sv) {
    std::string result;
    result.reserve(sv.size());
    bool inSpace = false;

    for (char c : sv) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!inSpace && !result.empty()) {
                result.push_back(' ');
                inSpace = true;
            }
        } else {
            result.push_back(c);
            inSpace = false;
        }
    }
    while (!result.empty() && std::isspace(static_cast<unsigned char>(result.back()))) {
        result.pop_back();
    }
    return result;
}

std::string FieldValueNormalizer::toUpper(std::string_view sv) {
    std::string s(sv);
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return s;
}

bool FieldValueNormalizer::startsWithCi(std::string_view str, std::string_view prefix) {
    if (prefix.size() > str.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (std::toupper(static_cast<unsigned char>(str[i])) !=
            std::toupper(static_cast<unsigned char>(prefix[i]))) {
            return false;
        }
    }
    return true;
}

NormalizationResult FieldValueNormalizer::normalize(
    const std::string& fieldName,
    std::string_view rawValue,
    std::string_view knownLabel
) {
    if (fieldName == "student_id") {
        return normalizeStudentId(rawValue, knownLabel);
    } else if (fieldName == "birthdate" || fieldName == "conferral_date" || fieldName == "date" || fieldName == "dob") {
        return normalizeDate(rawValue, knownLabel);
    } else if (fieldName == "course_code") {
        return normalizeCourseCode(rawValue);
    } else if (fieldName == "course_title") {
        return normalizeCourseTitle(rawValue);
    } else if (fieldName == "grade") {
        return normalizeGrade(rawValue);
    } else if (fieldName == "gpa" || fieldName == "credits" || fieldName == "quality_points") {
        return normalizeNumeric(rawValue);
    } else {
        return normalizeGenericMetadata(rawValue, knownLabel);
    }
}

NormalizationResult FieldValueNormalizer::normalizeDate(
    std::string_view raw,
    std::string_view label
) {
    NormalizationResult res;
    res.originalValue = std::string(raw);

    std::string cur = trim(raw);
    if (cur != res.originalValue) {
        res.operations.push_back("TRIM_WHITESPACE");
    }

    // 1. Remove known date label prefixes if present
    static const std::vector<std::string> dateLabels = {
        "BIRTHDATE", "DATE OF BIRTH", "DOB", "BIRTH DATE",
        "CONFERRAL DATE", "DATE CONFERRED", "GRADUATION DATE", "AWARD DATE", "DATE"
    };

    if (!label.empty() && startsWithCi(cur, label)) {
        cur = cur.substr(label.size());
        cur = trim(cur);
        res.operations.push_back("TRIM_LABEL_PREFIX");
    } else {
        for (const auto& l : dateLabels) {
            if (startsWithCi(cur, l)) {
                cur = cur.substr(l.size());
                cur = trim(cur);
                res.operations.push_back("TRIM_LABEL_PREFIX");
                break;
            }
        }
    }

    // 2. Strip leading label separators
    while (!cur.empty() && (cur.front() == ':' || cur.front() == '-' || cur.front() == '=' ||
                           cur.front() == '~' || cur.front() == '.' ||
                           std::isspace(static_cast<unsigned char>(cur.front())))) {
        cur.erase(0, 1);
        cur = trim(cur);
        res.operations.push_back("TRIM_LABEL_SEPARATOR");
    }

    static const std::unordered_map<std::string, std::string> monthMap = {
        {"JAN", "Jan"}, {"JANUARY", "Jan"}, {"01", "Jan"}, {"1", "Jan"},
        {"FEB", "Feb"}, {"FEBRUARY", "Feb"}, {"02", "Feb"}, {"2", "Feb"},
        {"MAR", "Mar"}, {"MARCH", "Mar"}, {"03", "Mar"}, {"3", "Mar"},
        {"APR", "Apr"}, {"APRIL", "Apr"}, {"04", "Apr"}, {"4", "Apr"},
        {"MAY", "May"}, {"05", "May"}, {"5", "May"},
        {"JUN", "Jun"}, {"JUNE", "Jun"}, {"06", "Jun"}, {"6", "Jun"},
        {"JUL", "Jul"}, {"JULY", "Jul"}, {"07", "Jul"}, {"7", "Jul"},
        {"AUG", "Aug"}, {"AUGUST", "Aug"}, {"08", "Aug"}, {"8", "Aug"},
        {"SEP", "Sep"}, {"SEPT", "Sep"}, {"SEPTEMBER", "Sep"}, {"09", "Sep"}, {"9", "Sep"},
        {"OCT", "Oct"}, {"OCTOBER", "Oct"}, {"10", "Oct"},
        {"NOV", "Nov"}, {"NOVEMBER", "Nov"}, {"11", "Nov"},
        {"DEC", "Dec"}, {"DECEMBER", "Dec"}, {"12", "Dec"}
    };

    auto lookupMonth = [](std::string m) -> std::string {
        std::string upperM;
        for (char c : m) upperM.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        auto it = monthMap.find(upperM);
        if (it != monthMap.end()) return it->second;
        return "";
    };

    std::smatch m;

    // Pattern 1: Merged month name + day + 4-digit year (e.g. "Jun 261997", "June 261997")
    static const std::regex mergedPattern(R"(^([A-Za-z]{3,9})\s*([0-9]{1,2})([12][0-9]{3})$)");

    // Pattern 2: Month name + space/comma + day + space/comma + 4-digit year (e.g. "Jun 26 1997", "June 26, 1997")
    static const std::regex textDatePattern(R"(^([A-Za-z]{3,9})\s+([0-9]{1,2})(?:st|nd|rd|th)?[\s,]+([12][0-9]{3})$)");

    // Pattern 3: Day + Month name + 4-digit year (e.g. "26 Jun 1997", "26 June 1997")
    static const std::regex dmyPattern(R"(^([0-9]{1,2})(?:st|nd|rd|th)?\s+([A-Za-z]{3,9})[\s,]+([12][0-9]{3})$)");

    // Pattern 4: Numeric MM/DD/YYYY or MM-DD-YYYY (e.g. "06/26/1997", "06-26-1997")
    static const std::regex numericPattern(R"(^([0-9]{1,2})[/\-.]([0-9]{1,2})[/\-.]([12][0-9]{3})$)");

    // Pattern 5: ISO YYYY-MM-DD or YYYY/MM/DD (e.g. "1997-06-26")
    static const std::regex isoPattern(R"(^([12][0-9]{3})[/\-.]([0-9]{1,2})[/\-.]([0-9]{1,2})$)");

    std::string normMonth;
    std::string normDay;
    std::string normYear;

    if (std::regex_match(cur, m, mergedPattern)) {
        normMonth = lookupMonth(m.str(1));
        normDay = std::to_string(std::stoi(m.str(2)));
        normYear = m.str(3);
        res.operations.push_back("SPLIT_MERGED_DAY_YEAR");
    } else if (std::regex_match(cur, m, textDatePattern)) {
        normMonth = lookupMonth(m.str(1));
        normDay = std::to_string(std::stoi(m.str(2)));
        normYear = m.str(3);
    } else if (std::regex_match(cur, m, dmyPattern)) {
        normMonth = lookupMonth(m.str(2));
        normDay = std::to_string(std::stoi(m.str(1)));
        normYear = m.str(3);
    } else if (std::regex_match(cur, m, numericPattern)) {
        int mVal = std::stoi(m.str(1));
        int dVal = std::stoi(m.str(2));
        if (mVal >= 1 && mVal <= 12 && dVal >= 1 && dVal <= 31) {
            normMonth = lookupMonth(std::to_string(mVal));
            normDay = std::to_string(dVal);
            normYear = m.str(3);
        }
    } else if (std::regex_match(cur, m, isoPattern)) {
        int mVal = std::stoi(m.str(2));
        int dVal = std::stoi(m.str(3));
        if (mVal >= 1 && mVal <= 12 && dVal >= 1 && dVal <= 31) {
            normMonth = lookupMonth(std::to_string(mVal));
            normDay = std::to_string(dVal);
            normYear = m.str(1);
        }
    }

    if (!normMonth.empty() && !normDay.empty() && !normYear.empty()) {
        res.normalizedValue = normMonth + " " + normDay + " " + normYear;
        res.operations.push_back("NORMALIZE_DATE");
        res.applied = (res.normalizedValue != res.originalValue);
        return res;
    }

    // Fall back to generic metadata normalization if non-standard date format
    return normalizeGenericMetadata(cur, label);
}

NormalizationResult FieldValueNormalizer::normalizeStudentId(
    std::string_view raw,
    std::string_view label
) {
    NormalizationResult res;
    res.originalValue = std::string(raw);

    std::string cur = trim(raw);
    if (cur != res.originalValue) {
        res.operations.push_back("TRIM_WHITESPACE");
    }

    // 1. Remove known label prefixes if present in the value itself
    static const std::vector<std::string> idLabels = {
        "STUDENT ID", "STUDENT ID NUMBER", "STUDENT ID NO", "STUDENT NUMBER",
        "STUDENT NO", "STUDENT", "ID NUMBER", "ID NO", "ID #", "ID#", "ID"
    };

    bool strippedPrefix = false;
    for (const auto& l : idLabels) {
        if (startsWithCi(cur, l)) {
            cur = cur.substr(l.size());
            cur = trim(cur);
            res.operations.push_back("TRIM_LABEL_PREFIX");
            strippedPrefix = true;
            break;
        }
    }

    // 2. Remove leading label separators (e.g. ":", "-", "=", "—", "~")
    while (!cur.empty() && (cur.front() == ':' || cur.front() == '-' || cur.front() == '=' ||
                           cur.front() == '.' || cur.front() == '#' ||
                           std::isspace(static_cast<unsigned char>(cur.front())))) {
        // If it starts with a hyphen followed by a digit without space, check if it's a separator or negative number
        // For student ID, leading dashes/colons before an ID are label separators
        if (cur.front() == ':' || cur.front() == '=' || cur.front() == '#') {
            cur.erase(0, 1);
            cur = trim(cur);
            res.operations.push_back("TRIM_LABEL_SEPARATOR");
        } else if (cur.front() == '-') {
            // Check if it's a leading dash separator, e.g. "- 712398456"
            cur.erase(0, 1);
            cur = trim(cur);
            res.operations.push_back("TRIM_LABEL_SEPARATOR");
        } else {
            cur.erase(0, 1);
            cur = trim(cur);
        }
    }

    // 3. Normalize internal spaces
    std::string finalVal = normalizeWhitespace(cur);
    if (finalVal != cur) {
        res.operations.push_back("NORMALIZE_SPACES");
    }

    res.normalizedValue = finalVal;
    res.applied = (res.normalizedValue != res.originalValue);
    return res;
}

NormalizationResult FieldValueNormalizer::normalizeCourseCode(std::string_view raw) {
    NormalizationResult res;
    res.originalValue = std::string(raw);

    std::string cur = trim(raw);
    if (cur != res.originalValue) {
        res.operations.push_back("TRIM_WHITESPACE");
    }

    // Normalize multiple spaces into single space (e.g., "ECON   001" -> "ECON 001")
    std::string finalVal = normalizeWhitespace(cur);
    if (finalVal != cur) {
        res.operations.push_back("NORMALIZE_SPACES");
    }

    res.normalizedValue = finalVal;
    res.applied = (res.normalizedValue != res.originalValue);
    return res;
}

NormalizationResult FieldValueNormalizer::normalizeCourseTitle(std::string_view raw) {
    NormalizationResult res;
    res.originalValue = std::string(raw);

    std::string cur = trim(raw);
    if (cur != res.originalValue) {
        res.operations.push_back("TRIM_WHITESPACE");
    }

    // Normalize multiple spaces to single space, preserving punctuation and word formatting
    std::string finalVal = normalizeWhitespace(cur);
    if (finalVal != cur) {
        res.operations.push_back("NORMALIZE_SPACES");
    }

    res.normalizedValue = finalVal;
    res.applied = (res.normalizedValue != res.originalValue);
    return res;
}

NormalizationResult FieldValueNormalizer::normalizeGrade(std::string_view raw) {
    NormalizationResult res;
    res.originalValue = std::string(raw);

    std::string cur = trim(raw);
    if (cur != res.originalValue) {
        res.operations.push_back("TRIM_WHITESPACE");
    }

    // Strip leading or trailing colon/delimiter if present
    while (!cur.empty() && (cur.front() == ':' || cur.front() == '=')) {
        cur.erase(0, 1);
        cur = trim(cur);
        res.operations.push_back("TRIM_LABEL_SEPARATOR");
    }

    std::string finalVal = normalizeWhitespace(cur);
    res.normalizedValue = finalVal;
    res.applied = (res.normalizedValue != res.originalValue);
    return res;
}

NormalizationResult FieldValueNormalizer::normalizeNumeric(std::string_view raw) {
    NormalizationResult res;
    res.originalValue = std::string(raw);

    std::string cur = trim(raw);
    if (cur != res.originalValue) {
        res.operations.push_back("TRIM_WHITESPACE");
    }

    // Strip leading colon/separator if present (e.g. ": 3.811")
    while (!cur.empty() && (cur.front() == ':' || cur.front() == '=')) {
        cur.erase(0, 1);
        cur = trim(cur);
        res.operations.push_back("TRIM_LABEL_SEPARATOR");
    }

    std::string finalVal = normalizeWhitespace(cur);
    res.normalizedValue = finalVal;
    res.applied = (res.normalizedValue != res.originalValue);
    return res;
}

NormalizationResult FieldValueNormalizer::normalizeGenericMetadata(
    std::string_view raw,
    std::string_view label
) {
    NormalizationResult res;
    res.originalValue = std::string(raw);

    std::string cur = trim(raw);
    if (cur != res.originalValue) {
        res.operations.push_back("TRIM_WHITESPACE");
    }

    // If label is present or common labels exist in raw string, strip prefix
    if (!label.empty() && startsWithCi(cur, label)) {
        cur = cur.substr(label.size());
        cur = trim(cur);
        res.operations.push_back("TRIM_LABEL_PREFIX");
    }

    // Strip leading label separator
    while (!cur.empty() && (cur.front() == ':' || cur.front() == '-' || cur.front() == '=')) {
        cur.erase(0, 1);
        cur = trim(cur);
        res.operations.push_back("TRIM_LABEL_SEPARATOR");
    }

    std::string finalVal = normalizeWhitespace(cur);
    if (finalVal != cur) {
        res.operations.push_back("NORMALIZE_SPACES");
    }

    res.normalizedValue = finalVal;
    res.applied = (res.normalizedValue != res.originalValue);
    return res;
}

} // namespace edmo::evidence

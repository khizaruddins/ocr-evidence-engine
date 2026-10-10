#include <edmo/evidence/structure/CourseRowDetector.hpp>
#include <edmo/evidence/structure/PlaceholderDetector.hpp>

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

std::string sanitizeGrade(std::string g) {
    auto trimmed = trimWhitespace(g);
    if (trimmed.empty()) return "";
    std::string s(trimmed);
    if (s.size() == 1) {
        return std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(s[0]))));
    }
    if (s.size() == 2 && std::isalpha(static_cast<unsigned char>(s[0])) && std::islower(static_cast<unsigned char>(s[1]))) {
        return std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(s[0]))));
    }
    if (s.size() == 2 && std::isalpha(static_cast<unsigned char>(s[0])) && (s[1] == '+' || s[1] == '-')) {
        return std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])))) + s[1];
    }
    return s;
}

} // namespace

bool CourseRowDetector::isValidCourseCode(std::string_view code) const noexcept {
    auto trimmed = trimWhitespace(code);
    if (trimmed.empty() || PlaceholderDetector::isSeparatorLine(trimmed)) return false;

    std::string upper = toUpper(trimmed);
    if (upper.find("FALL") != std::string::npos || upper.find("SPRING") != std::string::npos ||
        upper.find("SUMMER") != std::string::npos || upper.find("WINTER") != std::string::npos ||
        upper.find("TOTAL") != std::string::npos || upper.find("CURRENT") != std::string::npos ||
        upper.find("CUMULATIVE") != std::string::npos || upper.find("PAGE") != std::string::npos ||
        upper.find("HONORS") != std::string::npos || upper.find("DEAN") != std::string::npos ||
        upper.find("DISTINCTION") != std::string::npos || upper.find("GRADE") != std::string::npos ||
        upper.find("GPA") != std::string::npos || upper.find("EHRS") != std::string::npos ||
        upper.find("QPTS") != std::string::npos || upper.find("TRANSCRIPT") != std::string::npos ||
        upper.find("COURSE") != std::string::npos || upper.find("SUBJ") != std::string::npos) {
        return false;
    }

    // Must not contain invalid characters like punctuation colons, periods, quotes
    if (trimmed.find(':') != std::string_view::npos || trimmed.find('.') != std::string_view::npos ||
        trimmed.find('"') != std::string_view::npos || trimmed.find('\'') != std::string_view::npos ||
        trimmed.find('\\') != std::string_view::npos || trimmed.find('/') != std::string_view::npos) {
        return false;
    }

    // Pattern: 2-5 letters/digits, optional space/dash, 2-4 digits, optional letter suffix
    // Examples: ECON 001, MATH 035, CS 101, CS-106A, 22BS103, BIO-101, CHEM 102A
    static const std::regex codeRegex(
        R"(^(?:[A-Z]{2,5}[-\s]?[0-9]{2,4}[A-Z]?|[0-9]{2}[A-Z]{2,4}[0-9]{2,4}[A-Z]?)$)",
        std::regex::icase
    );

    std::string s(trimmed);
    if (!std::regex_match(s, codeRegex)) return false;

    // Check for repetitive nonsense like "oooro00z" (3 identical adjacent letters)
    for (size_t i = 2; i < upper.size(); ++i) {
        if (upper[i] == upper[i-1] && upper[i] == upper[i-2] && std::isalpha(static_cast<unsigned char>(upper[i]))) {
            return false;
        }
    }

    return true;
}

bool CourseRowDetector::isCourseRow(std::string_view text) const noexcept {
    auto trimmed = trimWhitespace(text);
    if (trimmed.empty()) return false;

    if (PlaceholderDetector::isSeparatorLine(trimmed)) return false;

    // Course row starts with course code pattern
    static const std::regex rowRegex(
        R"(^(?:[A-Z]{2,5}[-\s]?[0-9]{2,4}[A-Z]?|[0-9]{2}[A-Z]{2,4}[0-9]{2,4}[A-Z]?)\s+.+\s+[0-9]+(?:\.[0-9]+)?(?:\s+(?:[A-F][+-]?|P|S|CR|NC|W|I|IN PROGRESS|AUDIT))?)",
        std::regex::icase
    );

    return std::regex_search(std::string(trimmed), rowRegex);
}

ParsedCourseRow CourseRowDetector::parseText(
    std::string_view lineText,
    float baseConfidence,
    const std::string& currentTerm
) const {
    ParsedCourseRow res;
    res.confidence = baseConfidence;
    res.term = currentTerm;

    auto trimmed = trimWhitespace(lineText);
    if (trimmed.empty() || PlaceholderDetector::isSeparatorLine(trimmed)) {
        return res;
    }

    std::string s(trimmed);

    // Form 1: Code + Title + Credits + Grade + QualityPoints (e.g. "ECON 001 Prin of Microeconomics 3.00 A 12.00")
    static const std::regex form1(
        R"(^([A-Z0-9]{2,5}[-\s]?[0-9]{2,4}[A-Z]?)\s+(.+?)\s+([0-9]+(?:\.[0-9]+)?)\s+([A-F][+-]?|[A-F][a-z]?|P|S|CR|NC|W|I|AUDIT)\s+([0-9]+(?:\.[0-9]+)?)(?:\s+[A-Za-z0-9]+)?$)",
        std::regex::icase
    );

    // Form 2: Code + Title + Credits + In Progress (e.g. "ECON 002 Prin of Macroeconomics 3.00 In Progress")
    static const std::regex form2(
        R"(^([A-Z0-9]{2,5}[-\s]?[0-9]{2,4}[A-Z]?)\s+(.+?)\s+([0-9]+(?:\.[0-9]+)?)\s+(In\s+Progress|IN\s+PROGRESS)(?:\s+[0-9]+(?:\.[0-9]+)?)?$)",
        std::regex::icase
    );

    // Form 3: Code + Title + Credits + Grade (e.g. "CS 106A Programming Methodologies 5.0 S")
    static const std::regex form3(
        R"(^([A-Z0-9]{2,5}[-\s]?[0-9]{2,4}[A-Z]?)\s+(.+?)\s+([0-9]+(?:\.[0-9]+)?)\s+([A-F][+-]?|[A-F][a-z]?|P|S|CR|NC|W|I|AUDIT)(?:\s+[A-Za-z0-9]+)?$)",
        std::regex::icase
    );

    // Form 4: Code + Title + Credits (e.g. "MATH 035 Calculus I 4.00")
    static const std::regex form4(
        R"(^([A-Z0-9]{2,5}[-\s]?[0-9]{2,4}[A-Z]?)\s+(.+?)\s+([0-9]+(?:\.[0-9]+)?)$)",
        std::regex::icase
    );

    // Form 5: Code + Title (e.g. "CS 101 Intro to Computing")
    static const std::regex form5(
        R"(^([A-Z0-9]{2,5}[-\s]?[0-9]{2,4}[A-Z]?)\s+(.+)$)",
        std::regex::icase
    );

    std::smatch m;
    if (std::regex_match(s, m, form1) && m.size() >= 6) {
        res.isValid = true;
        res.courseCode = m.str(1);
        res.courseTitle = m.str(2);
        res.credits = m.str(3);
        res.grade = sanitizeGrade(m.str(4));
        res.qualityPoints = m.str(5);
        res.status = "COMPLETED";
    } else if (std::regex_match(s, m, form2) && m.size() >= 5) {
        res.isValid = true;
        res.courseCode = m.str(1);
        res.courseTitle = m.str(2);
        res.credits = m.str(3);
        res.grade = ""; // Grade is null/empty for in-progress courses
        res.status = "IN_PROGRESS";
    } else if (std::regex_match(s, m, form3) && m.size() >= 5) {
        res.isValid = true;
        res.courseCode = m.str(1);
        res.courseTitle = m.str(2);
        res.credits = m.str(3);
        res.grade = sanitizeGrade(m.str(4));
        res.status = "COMPLETED";
    } else if (std::regex_match(s, m, form4) && m.size() >= 4) {
        res.isValid = true;
        res.courseCode = m.str(1);
        res.courseTitle = m.str(2);
        res.credits = m.str(3);
        res.status = "REGISTERED";
    } else if (std::regex_match(s, m, form5) && m.size() >= 3) {
        res.isValid = true;
        res.courseCode = m.str(1);
        res.courseTitle = m.str(2);
        res.status = "REGISTERED";
    }

    if (res.isValid) {
        res.codeConfidence = baseConfidence;
        res.titleConfidence = baseConfidence;
        res.creditsConfidence = baseConfidence;
        res.gradeConfidence = baseConfidence;

        // Build nearby context bundle
        if (!res.courseCode.empty()) res.nearbyEvidence.push_back("Code: " + res.courseCode);
        if (!res.credits.empty()) res.nearbyEvidence.push_back("Credits: " + res.credits);
        if (!res.grade.empty()) res.nearbyEvidence.push_back("Grade: " + res.grade);
        if (!res.status.empty() && res.status != "COMPLETED") res.nearbyEvidence.push_back("Status: " + res.status);
        if (!res.term.empty()) res.nearbyEvidence.push_back("Term: " + res.term);
    }

    return res;
}

ParsedCourseRow CourseRowDetector::parseRow(
    const EvidenceRow& row,
    const std::string& currentTerm
) const {
    ParsedCourseRow res;
    res.confidence = row.confidence;
    res.term = currentTerm;

    if (row.isHeader || row.empty()) return res;

    // Check if canonical fields already present
    auto getField = [&](const std::string& key) -> std::string {
        if (auto it = row.canonicalFields.find(key); it != row.canonicalFields.end()) {
            return it->second;
        }
        return "";
    };

    std::string cCode = getField("course_code");
    std::string cTitle = getField("course_title");
    std::string cCredits = getField("credits");
    std::string cGrade = getField("grade");
    std::string cPoints = getField("quality_points");
    std::string cTerm = getField("term");

    // Check cells directly if any canonical field is missing
    if (cCode.empty()) {
        for (const auto& cell : row.cells) {
            std::string canUpper = toUpper(cell.canonicalColumnName);
            std::string colUpper = toUpper(cell.columnName);
            if (canUpper == "COURSE_CODE" || colUpper == "COURSE_CODE" || colUpper == "CODE" || colUpper == "CRS") {
                cCode = std::string(trimWhitespace(cell.text));
                break;
            }
        }
    }
    if (cTitle.empty()) {
        for (const auto& cell : row.cells) {
            std::string canUpper = toUpper(cell.canonicalColumnName);
            std::string colUpper = toUpper(cell.columnName);
            if (canUpper == "COURSE_TITLE" || colUpper == "COURSE_TITLE" || colUpper == "TITLE" || colUpper == "DESCRIPTION") {
                cTitle = std::string(trimWhitespace(cell.text));
                break;
            }
        }
    }
    if (cCredits.empty()) {
        for (const auto& cell : row.cells) {
            std::string canUpper = toUpper(cell.canonicalColumnName);
            std::string colUpper = toUpper(cell.columnName);
            if (canUpper == "CREDITS" || colUpper == "CREDITS" || colUpper == "CR" || colUpper == "HRS" || colUpper == "HOURS" || colUpper == "UNIT" || colUpper == "UNITS") {
                cCredits = std::string(trimWhitespace(cell.text));
                break;
            }
        }
    }
    if (cGrade.empty()) {
        for (const auto& cell : row.cells) {
            std::string canUpper = toUpper(cell.canonicalColumnName);
            std::string colUpper = toUpper(cell.columnName);
            if (canUpper == "GRADE" || colUpper == "GRADE" || colUpper == "GRD" || colUpper == "GR" ||
                colUpper == "FINAL GRADE" || colUpper == "FINAL_GRADE" || colUpper == "MARK" || colUpper == "MARKS") {
                cGrade = sanitizeGrade(std::string(trimWhitespace(cell.text)));
                break;
            }
        }
    }

    // Positional fallback if row has structured cells
    if (row.cells.size() >= 3) {
        std::string cell0 = std::string(trimWhitespace(row.cells[0].text));
        if (isValidCourseCode(cell0)) {
            if (cCode.empty()) cCode = cell0;
            if (cTitle.empty() && row.cells.size() > 1) cTitle = std::string(trimWhitespace(row.cells[1].text));
            if (cCredits.empty() && row.cells.size() > 2) cCredits = std::string(trimWhitespace(row.cells[2].text));
            if (cGrade.empty() && row.cells.size() > 3) cGrade = sanitizeGrade(std::string(trimWhitespace(row.cells[3].text)));
        }
    }

    if (cTerm.empty()) cTerm = currentTerm;

    if (!cCode.empty() && isValidCourseCode(cCode)) {
        res.isValid = true;
        res.courseCode = cCode;
        res.courseTitle = cTitle;
        res.credits = cCredits;
        res.grade = cGrade;
        res.qualityPoints = cPoints;
        res.term = cTerm;

        // If grade is still empty, inspect row.cells for any cell matching a valid grade pattern
        if (res.grade.empty()) {
            static const std::regex validGradeRegex(R"(^(?:[A-F][+-]?|[A-F][a-z]?|P|S|CR|NC|W|I|AUDIT)$)", std::regex::icase);
            for (size_t ci = 1; ci < row.cells.size(); ++ci) {
                auto trimmed = std::string(trimWhitespace(row.cells[ci].text));
                if (std::regex_match(trimmed, validGradeRegex)) {
                    res.grade = sanitizeGrade(trimmed);
                    res.gradeConfidence = row.cells[ci].confidence > 0.0f ? row.cells[ci].confidence : row.confidence;
                    break;
                }
            }
        }

        // If credits or grade are missing from canonical fields, attempt text regex parse
        if (res.credits.empty() || res.grade.empty()) {
            std::string fullText;
            for (size_t i = 0; i < row.cells.size(); ++i) {
                if (i > 0) fullText += " ";
                fullText += row.cells[i].text;
            }
            if (fullText.empty()) {
                fullText = cCode + " " + cTitle;
                if (!cCredits.empty()) fullText += " " + cCredits;
                if (!cGrade.empty()) fullText += " " + cGrade;
            }
            auto parsedText = parseText(fullText, row.confidence, cTerm);
            if (parsedText.isValid) {
                if (!parsedText.courseTitle.empty()) {
                    if (res.courseTitle.empty() || (!parsedText.credits.empty() && res.courseTitle.find(parsedText.credits) != std::string::npos)) {
                        res.courseTitle = parsedText.courseTitle;
                    }
                }
                if (!parsedText.credits.empty() && res.credits.empty()) res.credits = parsedText.credits;
                if (!parsedText.grade.empty() && res.grade.empty()) res.grade = parsedText.grade;
                if (!parsedText.qualityPoints.empty() && res.qualityPoints.empty()) res.qualityPoints = parsedText.qualityPoints;
                if (!parsedText.status.empty()) res.status = parsedText.status;
            }
        }

        res.codeConfidence = row.confidence > 0.0f ? row.confidence : 0.85f;
        res.titleConfidence = row.confidence > 0.0f ? row.confidence : 0.85f;
        res.creditsConfidence = row.confidence > 0.0f ? row.confidence : 0.85f;
        res.gradeConfidence = row.confidence > 0.0f ? row.confidence : 0.85f;

        // Use specific cell confidences if available
        for (const auto& cell : row.cells) {
            std::string canUpper = toUpper(cell.canonicalColumnName);
            std::string colUpper = toUpper(cell.columnName);
            if (canUpper == "GRADE" || colUpper == "GRADE" || cell.text == res.grade) {
                if (cell.confidence > 0.0f) res.gradeConfidence = cell.confidence;
            }
            if (canUpper == "COURSE_CODE" || cell.text == res.courseCode) {
                if (cell.confidence > 0.0f) res.codeConfidence = cell.confidence;
            }
            if (canUpper == "COURSE_TITLE" || cell.text == res.courseTitle) {
                if (cell.confidence > 0.0f) res.titleConfidence = cell.confidence;
            }
            if (canUpper == "CREDITS" || cell.text == res.credits) {
                if (cell.confidence > 0.0f) res.creditsConfidence = cell.confidence;
            }
        }

        if (toUpper(res.grade) == "IN PROGRESS") {
            res.status = "IN_PROGRESS";
            res.grade = "";
        } else if (res.status.empty()) {
            res.status = "COMPLETED";
        }

        // Build nearby context bundle
        if (!res.courseCode.empty()) res.nearbyEvidence.push_back("Code: " + res.courseCode);
        if (!res.credits.empty()) res.nearbyEvidence.push_back("Credits: " + res.credits);
        if (!res.grade.empty()) res.nearbyEvidence.push_back("Grade: " + res.grade);
        if (!res.status.empty() && res.status != "COMPLETED") res.nearbyEvidence.push_back("Status: " + res.status);
        if (!res.term.empty()) res.nearbyEvidence.push_back("Term: " + res.term);

        return res;
    }

    // Check cells
    if (row.cells.size() >= 3) {
        std::string cell0 = std::string(trimWhitespace(row.cells[0].text));
        if (isValidCourseCode(cell0)) {
            res.isValid = true;
            res.courseCode = cell0;
            res.codeConfidence = row.cells[0].confidence;

            if (row.cells.size() > 1) {
                res.courseTitle = std::string(trimWhitespace(row.cells[1].text));
                res.titleConfidence = row.cells[1].confidence;
            }
            if (row.cells.size() > 2) {
                res.credits = std::string(trimWhitespace(row.cells[2].text));
                res.creditsConfidence = row.cells[2].confidence;
            }
            if (row.cells.size() > 3) {
                std::string gText = std::string(trimWhitespace(row.cells[3].text));
                if (toUpper(gText) == "IN PROGRESS") {
                    res.status = "IN_PROGRESS";
                    res.grade = "";
                } else {
                    res.grade = gText;
                    res.gradeConfidence = row.cells[3].confidence;
                    res.status = "COMPLETED";
                }
            }
            if (row.cells.size() > 4) {
                res.qualityPoints = std::string(trimWhitespace(row.cells[4].text));
            }
            return res;
        }
    }

    // If single cell or concatenated text, parse line text
    std::string fullText;
    for (size_t i = 0; i < row.cells.size(); ++i) {
        if (i > 0) fullText += " ";
        fullText += row.cells[i].text;
    }

    if (!fullText.empty()) {
        return parseText(fullText, row.confidence, currentTerm);
    }

    return res;
}

} // namespace edmo::evidence

#include <ocr/evidence/dictionary/DictionaryEngine.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

DictionaryEngine::DictionaryEngine() {
    loadEmbeddedDefaults();
}

std::string DictionaryEngine::normalize(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            result.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        }
    }
    return result;
}

void DictionaryEngine::loadEmbeddedDefaults() {
    // General / Academic Stop Words & Institutions
    static const std::vector<std::string> generalDefaults = {
        "THE", "OF", "AND", "TO", "A", "IN", "FOR", "IS", "ON", "THAT", "BY", "THIS", "WITH",
        "OXFORD", "CAMBRIDGE", "HARVARD", "STANFORD", "BOSTON", "UNIVERSITY", "COLLEGE",
        "OFFICIAL", "UNOFFICIAL", "PERMANENT", "RECORD", "COURSES", "STUDENT", "NAME"
    };
    for (const auto& w : generalDefaults) addTerm(w, DictionaryType::General);

    // Transcript Vocabulary
    static const std::vector<std::string> transcriptDefaults = {
        "TRANSCRIPT", "OFFICIAL TRANSCRIPT", "UNOFFICIAL TRANSCRIPT", "ACADEMIC TRANSCRIPT",
        "CREDITS", "CREDIT", "CREDITS EARNED", "CREDITS ATTEMPTED", "TOTAL CREDITS",
        "GRADE", "GRADES", "GRADE POINTS", "QUALITY POINTS", "GPA", "CUMULATIVE GPA", "CGPA",
        "SEMESTER", "TERM", "SESSION", "QUARTER", "FALL", "SPRING", "SUMMER", "WINTER",
        "COURSE", "COURSE CODE", "COURSE TITLE", "COURSE NUMBER", "DESCRIPTION", "SUBJECT",
        "STUDENT", "STUDENT NAME", "STUDENT ID", "BIRTHDATE", "DATE OF BIRTH", "DOB",
        "REGISTRAR", "DEAN", "MAJOR", "MINOR", "DEPARTMENT", "FACULTY", "COLLEGE", "SCHOOL",
        "PROGRAM", "DEGREE", "DEGREES AWARDED", "DEGREE CONFERRED", "CONFERRAL DATE", "GRADUATION DATE"
    };
    for (const auto& w : transcriptDefaults) addTerm(w, DictionaryType::Transcript);

    // Academic Vocabulary
    static const std::vector<std::string> academicDefaults = {
        "UNIVERSITY", "INSTITUTE", "INSTITUTION", "COLLEGE", "DEPARTMENT", "FACULTY", "SCHOOL",
        "OXFORD", "UNIVERSITY OF OXFORD", "BOSTON UNIVERSITY", "CAMBRIDGE UNIVERSITY", "GEORGETOWN", "GEORGETOWN UNIVERSITY",
        "ACCOUNTING", "FINANCIAL", "FINANCE", "ECONOMICS", "MICROECONOMICS", "MACROECONOMICS", "BUSINESS", "MANAGEMENT", "MARKETING", "COMMERCE",
        "AUDITING", "TAXATION", "MATHEMATICS", "CALCULUS", "ALGEBRA", "STATISTICS", "GEOMETRY",
        "COMPUTING", "COMPUTER", "SCIENCE", "COMPUTER SCIENCE", "SOFTWARE", "SOFTWARE ENGINEERING", "INFORMATION", "INFORMATION TECHNOLOGY",
        "SYSTEM", "SYSTEMS", "INFORMATION SYSTEMS", "DATA", "DATA SCIENCE", "CYBER SECURITY", "ARTIFICIAL INTELLIGENCE",
        "PHYSICS", "CHEMISTRY", "BIOLOGY", "PSYCHOLOGY", "HISTORY", "ENGLISH", "LAW", "MEDICINE", "STUDIES",
        "ENGINEERING", "CIVIL ENGINEERING", "MECHANICAL ENGINEERING", "ELECTRICAL ENGINEERING",
        "RESEARCH", "DEVELOPMENT", "ANALYSIS", "DESIGN", "ORGANIZATION", "PROFESSIONAL",
        "INTRODUCTION", "ADVANCED", "APPLIED", "PRACTICE", "THEORY", "PRINCIPLES", "PRIN", "FOUNDATIONS",
        "GENERAL", "HONORS", "SEMINAR", "LABORATORY", "LAB", "DISCRETE", "LINEAR", "ALGORITHMS", "STRUCTURES",
        "INTERMEDIATE", "ETHICS", "LITERATURE", "WRITING", "COMPOSITION", "READING"
    };
    for (const auto& w : academicDefaults) addTerm(w, DictionaryType::Academic);

    // Valid Grades
    static const std::vector<std::string> gradeDefaults = {
        "A+", "A", "A-", "B+", "B", "B-", "C+", "C", "C-", "D+", "D", "D-", "E", "F",
        "O", "O+", "P", "NP", "S", "U", "W", "WP", "WF", "I", "INC", "IP", "CR", "NC", "AUD", "AU", "EX", "TR"
    };
    for (const auto& g : gradeDefaults) addTerm(g, DictionaryType::Grades);

    // Degree types
    static const std::vector<std::string> degreeDefaults = {
        "BACHELORS", "BACHELOR", "BACHELOR OF SCIENCE", "BACHELOR OF ARTS", "BACHELOR OF COMMERCE",
        "BACHELOR IN ACCOUNTING & FINANCE", "MASTERS", "MASTER", "MASTER OF SCIENCE", "MASTER OF ARTS",
        "MASTER OF BUSINESS ADMINISTRATION", "DOCTOR OF PHILOSOPHY", "PH.D.", "DIPLOMA", "CERTIFICATE"
    };
    for (const auto& d : degreeDefaults) addTerm(d, DictionaryType::Degrees);
}

void DictionaryEngine::addTerm(const std::string& term, DictionaryType type) {
    if (term.empty()) return;
    std::string norm = normalize(term);

    switch (type) {
        case DictionaryType::General:
            generalWords_.insert(norm);
            break;
        case DictionaryType::Academic:
            academicWords_.insert(norm);
            domainTermsList_.push_back(term);
            break;
        case DictionaryType::Transcript:
            transcriptWords_.insert(norm);
            domainTermsList_.push_back(term);
            break;
        case DictionaryType::Grades:
            validGrades_.insert(norm);
            break;
        case DictionaryType::Degrees:
            degreesList_.insert(norm);
            domainTermsList_.push_back(term);
            break;
    }
}

bool DictionaryEngine::contains(std::string_view term, DictionaryType type) const noexcept {
    std::string norm = normalize(term);
    switch (type) {
        case DictionaryType::General:    return generalWords_.count(norm) > 0;
        case DictionaryType::Academic:   return academicWords_.count(norm) > 0;
        case DictionaryType::Transcript: return transcriptWords_.count(norm) > 0;
        case DictionaryType::Grades:     return validGrades_.count(norm) > 0;
        case DictionaryType::Degrees:    return degreesList_.count(norm) > 0;
    }
    return false;
}

bool DictionaryEngine::containsAny(std::string_view term) const noexcept {
    std::string norm = normalize(term);
    return transcriptWords_.count(norm) > 0 ||
           academicWords_.count(norm) > 0 ||
           degreesList_.count(norm) > 0 ||
           validGrades_.count(norm) > 0 ||
           generalWords_.count(norm) > 0;
}

float DictionaryEngine::scoreTerm(std::string_view term) const noexcept {
    std::string norm = normalize(term);
    if (transcriptWords_.count(norm) > 0 || degreesList_.count(norm) > 0) {
        return 1.0f;
    }
    if (academicWords_.count(norm) > 0) {
        return 0.98f;
    }
    if (validGrades_.count(norm) > 0) {
        return 1.0f;
    }
    if (generalWords_.count(norm) > 0) {
        return 0.85f;
    }

    // If multi-word phrase, score constituent words
    if (term.find(' ') != std::string_view::npos) {
        std::string termStr(term);
        std::istringstream iss(termStr);
        std::string word;
        int wordCount = 0;
        float sum = 0.0f;
        while (iss >> word) {
            wordCount++;
            sum += scoreTerm(word);
        }
        if (wordCount > 0) {
            return sum / wordCount;
        }
    }

    return 0.0f;
}

bool DictionaryEngine::isValidGrade(std::string_view grade) const noexcept {
    return validGrades_.count(normalize(grade)) > 0;
}

bool DictionaryEngine::loadDictionaryFile(const std::filesystem::path& filePath, DictionaryType type) {
    if (!std::filesystem::exists(filePath)) return false;
    try {
        std::ifstream in(filePath);
        nlohmann::json j;
        in >> j;

        if (j.is_object()) {
            if (j.contains("terms") && j["terms"].is_array()) {
                for (const auto& item : j["terms"]) {
                    if (item.is_string()) addTerm(item.get<std::string>(), type);
                }
            } else if (j.contains("valid_grades") && j["valid_grades"].is_array()) {
                for (const auto& item : j["valid_grades"]) {
                    if (item.is_string()) addTerm(item.get<std::string>(), type);
                }
            } else if (j.contains("degrees") && j["degrees"].is_array()) {
                for (const auto& item : j["degrees"]) {
                    if (item.is_string()) addTerm(item.get<std::string>(), type);
                }
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool DictionaryEngine::loadDictionariesFromDir(const std::filesystem::path& dirPath) {
    if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath)) {
        return false;
    }

    loadDictionaryFile(dirPath / "transcript.json", DictionaryType::Transcript);
    loadDictionaryFile(dirPath / "academic.json", DictionaryType::Academic);
    loadDictionaryFile(dirPath / "grades.json", DictionaryType::Grades);
    loadDictionaryFile(dirPath / "degrees.json", DictionaryType::Degrees);
    loadDictionaryFile(dirPath / "general.json", DictionaryType::General);
    return true;
}

} // namespace ocr::evidence

#include <edmo/evidence/projection/FinalDocumentProjector.hpp>
#include <edmo/evidence/parser/EdrParser.hpp>

#include <algorithm>
#include <cctype>
#include <map>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

namespace edmo::evidence {

namespace {

inline std::string toUpper(std::string_view sv) {
    std::string res;
    res.reserve(sv.size());
    for (char c : sv) {
        res.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return res;
}

inline std::string_view trimWhitespace(std::string_view sv) noexcept {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }
    return sv;
}

inline std::string sanitizeGrade(std::string_view g) {
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

std::string extractIdSuffix(const std::string& fieldId) {
    auto pos = fieldId.rfind('_');
    if (pos != std::string::npos && pos + 1 < fieldId.size()) {
        return fieldId.substr(pos + 1);
    }
    return fieldId;
}

} // namespace

nlohmann::json FinalDocumentProjector::project(
    const EnhancedEdrResult& enhancedResult,
    const EvidenceDocument& originalDoc
) {
    nlohmann::json doc = nlohmann::json::object();
    doc["schema_version"] = "edmo-final-document-0.1";

    std::string docType = enhancedResult.documentType.empty() ? "transcript" : enhancedResult.documentType;
    int pageCount = originalDoc.metadata.pageCount > 0 ? originalDoc.metadata.pageCount : 1;

    doc["document"] = {
        {"document_id", enhancedResult.documentId},
        {"document_type", docType},
        {"page_count", pageCount}
    };
    // Root level backward compatibility keys
    doc["document_type"] = docType;
    doc["document_id"] = enhancedResult.documentId;

    // 1. Identity & Student info
    nlohmann::json student = nlohmann::json::object();
    student["name"] = nullptr;
    student["student_id"] = nullptr;
    student["birthdate"] = nullptr;

    for (const auto& [k, v] : originalDoc.identity) {
        if (!v.empty()) {
            if (k == "birthdate") {
                student[k] = FieldValueNormalizer::normalize("birthdate", v, "BIRTHDATE").normalizedValue;
            } else {
                student[k] = v;
            }
        }
    }

    // 2. Institution info
    nlohmann::json inst = nlohmann::json::object();
    inst["name"] = nullptr;
    for (const auto& [k, v] : originalDoc.institution) {
        if (!v.empty()) {
            inst[k] = v;
        }
    }

    // 3. Program info
    nlohmann::json prog = nlohmann::json::object();
    prog["degree"] = nullptr;
    prog["major"] = nullptr;
    prog["minor"] = nullptr;
    prog["school"] = nullptr;
    prog["department"] = nullptr;
    for (const auto& [k, v] : originalDoc.academic) {
        if (!v.empty()) {
            prog[k] = v;
        }
    }

    // 4. Course Records grouping
    struct CourseItem {
        std::string code;
        std::string title;
        std::string credits;
        std::string grade;
        std::string points;
        std::string term;
        std::string status = "COMPLETED";
        float confidence = 1.0f;
        std::string resolutionMethod = "OCR";
        EvidenceSource source;
    };

    std::map<std::string, CourseItem> courseMap;
    std::vector<std::string> courseOrder;

    nlohmann::json summary = nlohmann::json::object();
    summary["gpa"] = nullptr;
    summary["cumulative_gpa"] = nullptr;
    summary["term_gpa"] = nullptr;
    summary["total_credits_earned"] = nullptr;

    float sumConf = 0.0f;
    float minConf = 1.0f;
    int semanticFieldCount = 0;

    // Section confidence trackers for WEIGHTED_SEMANTIC_FIELDS calculation
    float instConfSum = 0.0f;
    int instCount = 0;
    float studentConfSum = 0.0f;
    int studentCount = 0;
    float progConfSum = 0.0f;
    int progCount = 0;
    float courseConfSum = 0.0f;
    int courseFieldCount = 0;
    float summaryConfSum = 0.0f;
    int summaryCount = 0;

    for (const auto& f : enhancedResult.fields) {
        if (!f.isSemanticValid) continue;

        semanticFieldCount++;
        sumConf += f.final.confidence;
        if (f.final.confidence < minConf) minConf = f.final.confidence;

        if (f.field == "student_name") {
            student["name"] = f.final.value;
            student["student_name"] = f.final.value;
            studentConfSum += f.final.confidence;
            studentCount++;
        } else if (f.field == "student_id") {
            student["student_id"] = f.final.value;
            studentConfSum += f.final.confidence;
            studentCount++;
        } else if (f.field == "birthdate") {
            student["birthdate"] = f.final.value;
            studentConfSum += f.final.confidence;
            studentCount++;
        } else if (f.field == "institution_name") {
            inst["name"] = f.final.value;
            instConfSum += f.final.confidence;
            instCount++;
        } else if (f.field == "degree") {
            prog["degree"] = f.final.value;
            progConfSum += f.final.confidence;
            progCount++;
        } else if (f.field == "major") {
            prog["major"] = f.final.value;
            progConfSum += f.final.confidence;
            progCount++;
        } else if (f.field == "minor") {
            prog["minor"] = f.final.value;
            progConfSum += f.final.confidence;
            progCount++;
        } else if (f.field == "department") {
            prog["department"] = f.final.value;
            progConfSum += f.final.confidence;
            progCount++;
        } else if (f.field == "school") {
            prog["school"] = f.final.value;
            progConfSum += f.final.confidence;
            progCount++;
        } else if (f.field == "gpa" || f.field == "cumulative_gpa") {
            try {
                summary["cumulative_gpa"] = std::stof(f.final.value);
                summary["gpa"] = std::stof(f.final.value);
            } catch (...) {
                summary["cumulative_gpa"] = f.final.value;
                summary["gpa"] = f.final.value;
            }
            summaryConfSum += f.final.confidence;
            summaryCount++;
        } else if (f.field == "term_gpa") {
            try {
                summary["term_gpa"] = std::stof(f.final.value);
            } catch (...) {
                summary["term_gpa"] = f.final.value;
            }
            summaryConfSum += f.final.confidence;
            summaryCount++;
        } else if (f.field == "total_credits" || f.field == "total_credits_earned") {
            try {
                summary["total_credits_earned"] = std::stof(f.final.value);
            } catch (...) {
                summary["total_credits_earned"] = f.final.value;
            }
            summaryConfSum += f.final.confidence;
            summaryCount++;
        } else if (f.category == "course_record") {
            std::string key;
            static const std::regex numSuffix(R"(.*_([0-9]+)$)");
            std::smatch sm;
            if (std::regex_match(f.fieldId, sm, numSuffix) && sm.size() >= 2) {
                key = "row_" + sm.str(1);
            } else {
                std::string tbl = f.labelBlockId.empty() ? f.original.source.tableId : f.labelBlockId;
                if (!tbl.empty() && f.original.source.row >= 0) {
                    key = tbl + "_r" + std::to_string(f.original.source.row);
                } else if (f.original.source.page > 0 && f.original.source.bbox.y > 0) {
                    for (const auto& k : courseOrder) {
                        const auto& existing = courseMap[k];
                        if (existing.source.page == f.original.source.page && existing.source.bbox.y > 0) {
                            int dy = std::abs((existing.source.bbox.y + existing.source.bbox.height / 2) -
                                              (f.original.source.bbox.y + f.original.source.bbox.height / 2));
                            if (dy <= 35) {
                                key = k;
                                break;
                            }
                        }
                    }
                }
                if (key.empty()) {
                    key = extractIdSuffix(f.fieldId);
                }
            }

            if (courseMap.count(key) == 0) {
                courseOrder.push_back(key);
            }
            auto& c = courseMap[key];
            if (!f.term.empty()) c.term = f.term;
            if (!f.status.empty()) c.status = f.status;
            if (f.final.confidence < c.confidence) c.confidence = f.final.confidence;
            if (f.resolutionMethod == "LLM" || c.resolutionMethod == "LLM") c.resolutionMethod = "LLM";
            else if (f.correction.applied || c.resolutionMethod == "DETERMINISTIC") c.resolutionMethod = "DETERMINISTIC";
            if (c.source.empty() || f.field == "course_code") {
                c.source = f.original.source;
            }

            courseConfSum += f.final.confidence;
            courseFieldCount++;

            if (f.field == "course_code") c.code = f.final.value;
            else if (f.field == "course_title") c.title = f.final.value;
            else if (f.field == "credits") c.credits = f.final.value;
            else if (f.field == "grade") c.grade = sanitizeGrade(f.final.value);
            else if (f.field == "quality_points") c.points = f.final.value;
        }
    }

    nlohmann::json coursesArray = nlohmann::json::array();
    for (const auto& key : courseOrder) {
        const auto& c = courseMap[key];
        if (c.code.empty() && c.title.empty()) continue;

        nlohmann::json crsObj{
            {"course_code", c.code.empty() ? nullptr : nlohmann::json(c.code)},
            {"course_title", c.title.empty() ? nullptr : nlohmann::json(c.title)},
            {"term", c.term.empty() ? nullptr : nlohmann::json(c.term)},
            {"status", c.status},
            {"confidence", c.confidence},
            {"resolution_method", c.resolutionMethod},
            {"source", c.source.toJson(false)}
        };

        if (!c.credits.empty()) {
            try {
                crsObj["credits"] = std::stof(c.credits);
            } catch (...) {
                crsObj["credits"] = nullptr;
            }
        } else {
            crsObj["credits"] = nullptr;
        }

        // DETERMINISTIC DATA LOSS INVARIANT FOR CREDITS:
        // If a structured table row contains a valid credits cell,
        // then final projection MUST NOT silently convert it to null.
        if (crsObj["credits"].is_null()) {
            for (const auto& tbl : originalDoc.tables) {
                for (const auto& r : tbl.rows) {
                    bool matchesCourse = false;
                    if (!c.code.empty()) {
                        if (auto it = r.canonicalFields.find("course_code"); it != r.canonicalFields.end() && it->second == c.code) {
                            matchesCourse = true;
                        } else if (!r.cells.empty() && (r.cells[0].text == c.code || r.cells[0].text.rfind(c.code, 0) == 0)) {
                            matchesCourse = true;
                        }
                    }
                    if (!matchesCourse && !c.title.empty()) {
                        if (auto it = r.canonicalFields.find("course_title"); it != r.canonicalFields.end() && it->second == c.title) {
                            matchesCourse = true;
                        } else if (r.cells.size() > 1 && r.cells[1].text.find(c.title) != std::string::npos) {
                            matchesCourse = true;
                        }
                    }
                    if (!matchesCourse && !c.source.tableId.empty() && c.source.tableId == tbl.id && c.source.row >= 0 && r.rowIndex == c.source.row) {
                        matchesCourse = true;
                    }
                    if (!matchesCourse && !c.source.empty() && !r.source.empty() && r.source.page == c.source.page) {
                        if (std::abs(r.source.bbox.y - c.source.bbox.y) <= 35) {
                            matchesCourse = true;
                        }
                    }

                    if (matchesCourse) {
                        std::string structuredCredits;
                        if (auto cit = r.canonicalFields.find("credits"); cit != r.canonicalFields.end() && !cit->second.empty()) {
                            structuredCredits = cit->second;
                        }
                        if (structuredCredits.empty()) {
                            static const std::regex validCreditsRegex(R"(^[0-9]+(?:\.[0-9]+)?$)");
                            static const std::regex fusedCreditsRegex(R"(^.*?([0-9]+(?:\.[0-9]+)?)\s+(?:[A-F][+-]?|[A-F][a-z]?|P|S|CR|NC|W|I|AUDIT)$)", std::regex::icase);

                            for (const auto& cell : r.cells) {
                                std::string colUpper = toUpper(cell.columnName);
                                std::string canUpper = toUpper(cell.canonicalColumnName);
                                auto trimmed = std::string(trimWhitespace(cell.text));
                                if (canUpper == "CREDITS" || colUpper == "CREDITS" || colUpper == "CR" || colUpper == "HRS" || colUpper == "HOURS" || colUpper == "UNIT") {
                                    if (std::regex_match(trimmed, validCreditsRegex)) {
                                        structuredCredits = trimmed;
                                        break;
                                    }
                                }
                                std::smatch m;
                                if (std::regex_search(trimmed, m, fusedCreditsRegex) && m.size() >= 2) {
                                    structuredCredits = m.str(1);
                                    break;
                                }
                            }
                        }

                        if (!structuredCredits.empty()) {
                            try {
                                crsObj["credits"] = std::stof(structuredCredits);
                            } catch (...) {
                                crsObj["credits"] = nullptr;
                            }
                            break;
                        }
                    }
                }
                if (!crsObj["credits"].is_null()) break;
            }
        }

        if (!c.grade.empty()) {
            crsObj["grade"] = c.grade;
        } else {
            crsObj["grade"] = nullptr;
        }

        // DETERMINISTIC DATA LOSS INVARIANT:
        // If a structured table row contains a valid grade cell,
        // then final projection MUST NOT silently convert it to null.
        if (crsObj["grade"].is_null()) {
            for (const auto& tbl : originalDoc.tables) {
                for (const auto& r : tbl.rows) {
                    bool matchesCourse = false;
                    if (!c.code.empty()) {
                        if (auto it = r.canonicalFields.find("course_code"); it != r.canonicalFields.end() && it->second == c.code) {
                            matchesCourse = true;
                        } else if (!r.cells.empty() && (r.cells[0].text == c.code || r.cells[0].text.rfind(c.code, 0) == 0)) {
                            matchesCourse = true;
                        }
                    }
                    if (!matchesCourse && !c.title.empty()) {
                        if (auto it = r.canonicalFields.find("course_title"); it != r.canonicalFields.end() && it->second == c.title) {
                            matchesCourse = true;
                        } else if (r.cells.size() > 1 && r.cells[1].text.find(c.title) != std::string::npos) {
                            matchesCourse = true;
                        }
                    }
                    if (!matchesCourse && !c.source.tableId.empty() && c.source.tableId == tbl.id && c.source.row >= 0 && r.rowIndex == c.source.row) {
                        matchesCourse = true;
                    }
                    if (!matchesCourse && !c.source.empty() && !r.source.empty() && r.source.page == c.source.page) {
                        if (std::abs(r.source.bbox.y - c.source.bbox.y) <= 35) {
                            matchesCourse = true;
                        }
                    }

                    if (matchesCourse) {
                        std::string structuredGrade;
                        if (auto git = r.canonicalFields.find("grade"); git != r.canonicalFields.end() && !git->second.empty()) {
                            structuredGrade = git->second;
                        }
                        if (structuredGrade.empty()) {
                            static const std::regex validGradeRegex(R"(^(?:[A-F][+-]?|[A-F][a-z]?|P|S|CR|NC|W|I|AUDIT)$)", std::regex::icase);
                            static const std::regex fusedGradeRegex(R"((?:^|\s|\d+)([A-F][+-]?|[A-F][a-z]?|P|S|CR|NC|W|I|AUDIT)$)", std::regex::icase);

                            for (const auto& cell : r.cells) {
                                std::string colUpper = toUpper(cell.columnName);
                                std::string canUpper = toUpper(cell.canonicalColumnName);
                                bool isGradeCol = (canUpper == "GRADE" || colUpper == "GRADE" || colUpper == "GRD" || colUpper == "GR" ||
                                                   colUpper == "FINAL GRADE" || colUpper == "FINAL_GRADE" || colUpper == "MARK" || colUpper == "MARKS" ||
                                                   cell.fate == EvidenceFate::Structured);
                                auto trimmed = std::string(trimWhitespace(cell.text));
                                if (isGradeCol && std::regex_match(trimmed, validGradeRegex)) {
                                    structuredGrade = trimmed;
                                    break;
                                }
                                std::smatch m;
                                if (std::regex_search(trimmed, m, fusedGradeRegex) && m.size() >= 2) {
                                    structuredGrade = m.str(1);
                                }
                            }
                        }
                        if (structuredGrade.empty()) {
                            static const std::regex validGradeRegex(R"(^(?:[A-F][+-]?|[A-F][a-z]?|P|S|CR|NC|W|I|AUDIT)$)", std::regex::icase);
                            for (const auto& cell : r.cells) {
                                auto trimmed = std::string(trimWhitespace(cell.text));
                                if (std::regex_match(trimmed, validGradeRegex)) {
                                    structuredGrade = trimmed;
                                    break;
                                }
                            }
                        }

                        if (!structuredGrade.empty()) {
                            crsObj["grade"] = sanitizeGrade(structuredGrade);
                            if (crsObj["status"] == "REGISTERED" || crsObj["status"].is_null()) {
                                crsObj["status"] = "COMPLETED";
                            }
                            break;
                        }
                    }
                }
                if (!crsObj["grade"].is_null()) break;
            }
        }

        // Check horizontal aligned OCR blocks on page if still null
        if (crsObj["grade"].is_null() && !c.source.empty() && c.source.page > 0) {
            static const std::regex validGradeRegex(R"(^(?:[A-F][+-]?|[A-F][a-z]?|P|S|CR|NC|W|I|AUDIT)$)", std::regex::icase);
            for (const auto& p : originalDoc.pages) {
                if (p.pageNumber != c.source.page) continue;
                for (const auto& blk : p.blocks) {
                    auto trimmedBlk = std::string(trimWhitespace(blk.text));
                    if (std::regex_match(trimmedBlk, validGradeRegex)) {
                        int dy = std::abs(blk.source.bbox.y - c.source.bbox.y);
                        if (dy <= 30 && blk.source.bbox.x >= c.source.bbox.x + 100) {
                            crsObj["grade"] = sanitizeGrade(trimmedBlk);
                            if (crsObj["status"] == "REGISTERED" || crsObj["status"].is_null()) {
                                crsObj["status"] = "COMPLETED";
                            }
                            break;
                        }
                    }
                }
                if (!crsObj["grade"].is_null()) break;
            }
        }

        if (!c.points.empty()) {
            try {
                crsObj["quality_points"] = std::stof(c.points);
            } catch (...) {
                crsObj["quality_points"] = c.points;
            }
        } else {
            crsObj["quality_points"] = nullptr;
        }

        coursesArray.push_back(std::move(crsObj));
    }

    doc["institution"] = std::move(inst);
    doc["student"] = std::move(student);
    doc["program"] = std::move(prog);
    doc["courses"] = std::move(coursesArray);
    doc["summary"] = std::move(summary);

    // 5. Preserve unknown / additional sections (Part 5: Zero information loss)
    nlohmann::json additionalSections = nlohmann::json::array();
    if (!originalDoc.unclassifiedContent.empty()) {
        nlohmann::json sec{
            {"type", "unclassified_content"},
            {"title", "Additional Text & Content"},
            {"content", originalDoc.unclassifiedContent}
        };
        additionalSections.push_back(std::move(sec));
    }
    if (!additionalSections.empty()) {
        doc["additional_sections"] = std::move(additionalSections);
    }

    // 6. Weighted Semantic Confidence Calculation (Part 10)
    // Weights: Institution: 0.15, Student: 0.20, Program: 0.15, Courses: 0.40, Summary: 0.10
    float weightedNumerator = 0.0f;
    float weightDenominator = 0.0f;

    if (instCount > 0) {
        weightedNumerator += 0.15f * (instConfSum / instCount);
        weightDenominator += 0.15f;
    }
    if (studentCount > 0) {
        weightedNumerator += 0.20f * (studentConfSum / studentCount);
        weightDenominator += 0.20f;
    }
    if (progCount > 0) {
        weightedNumerator += 0.15f * (progConfSum / progCount);
        weightDenominator += 0.15f;
    }
    if (courseFieldCount > 0) {
        weightedNumerator += 0.40f * (courseConfSum / courseFieldCount);
        weightDenominator += 0.40f;
    }
    if (summaryCount > 0) {
        weightedNumerator += 0.10f * (summaryConfSum / summaryCount);
        weightDenominator += 0.10f;
    }

    float avgConf = (semanticFieldCount > 0) ? (sumConf / semanticFieldCount) : 1.0f;
    if (semanticFieldCount == 0) minConf = 1.0f;

    float overallConf = (weightDenominator > 0.0f) ? (weightedNumerator / weightDenominator) : avgConf;
    float extractionConf = originalDoc.confidence.tokenCount > 0 ? originalDoc.confidence.mean : 0.95f;
    float resolvedFieldConf = overallConf;

    std::string readiness = "READY";
    if (enhancedResult.unresolvedCount > 0) {
        if (enhancedResult.llmResolvedCount > 0) {
            readiness = "PARTIAL";
        } else {
            readiness = "RESOLUTION_PENDING";
        }
    }

    doc["document_readiness"] = readiness;

    doc["confidence"] = {
        {"overall", overallConf},
        {"extraction", extractionConf},
        {"resolved_fields", resolvedFieldConf},
        {"resolved_field", resolvedFieldConf},
        {"method", "WEIGHTED_SEMANTIC_FIELDS"},
        {"average", avgConf},
        {"minimum", minConf}
    };

    doc["resolution"] = {
        {"deterministic", enhancedResult.deterministicCount},
        {"ocr_confident", enhancedResult.ocrConfidentCount},
        {"normalized", enhancedResult.normalizedCount},
        {"llm_required", enhancedResult.patchReport.received > 0 ? enhancedResult.patchReport.received : (enhancedResult.llmResolvedCount + enhancedResult.unresolvedCount)},
        {"llm_resolved", enhancedResult.llmResolvedCount},
        {"ai_unresolved", enhancedResult.aiUnresolvedCount},
        {"unresolved", enhancedResult.unresolvedCount}
    };

    return doc;
}

nlohmann::json FinalDocumentProjector::projectFromEnhancedJson(
    const nlohmann::json& enhancedEdrJson
) {
    EnhancedEdrResult res;
    res.documentId = enhancedEdrJson.value("document_id", "");
    res.documentType = enhancedEdrJson.value("document_type", "transcript");

    EvidenceDocument doc;
    doc.metadata.id = res.documentId;
    doc.metadata.type = res.documentType;

    if (enhancedEdrJson.contains("identity") && enhancedEdrJson["identity"].is_object()) {
        for (const auto& [k, v] : enhancedEdrJson["identity"].items()) {
            if (v.is_string()) doc.identity[k] = v.get<std::string>();
        }
    }
    if (enhancedEdrJson.contains("institution") && enhancedEdrJson["institution"].is_object()) {
        for (const auto& [k, v] : enhancedEdrJson["institution"].items()) {
            if (v.is_string()) doc.institution[k] = v.get<std::string>();
        }
    }
    if (enhancedEdrJson.contains("academic") && enhancedEdrJson["academic"].is_object()) {
        for (const auto& [k, v] : enhancedEdrJson["academic"].items()) {
            if (v.is_string()) doc.academic[k] = v.get<std::string>();
        }
    }
    if (enhancedEdrJson.contains("unclassified_content") && enhancedEdrJson["unclassified_content"].is_array()) {
        for (const auto& item : enhancedEdrJson["unclassified_content"]) {
            if (item.is_string()) doc.unclassifiedContent.push_back(item.get<std::string>());
        }
    }

    if (enhancedEdrJson.contains("tables") && enhancedEdrJson["tables"].is_array()) {
        try {
            EvidenceDocument parsedDoc = EdrParser::parseJson(enhancedEdrJson);
            doc.tables = std::move(parsedDoc.tables);
            doc.pages = std::move(parsedDoc.pages);
        } catch (...) {}
    }

    if (enhancedEdrJson.contains("fields") && enhancedEdrJson["fields"].is_array()) {
        int idx = 1;
        for (const auto& fj : enhancedEdrJson["fields"]) {
            if (!fj.is_object()) continue;
            EvidenceField f;
            f.fieldId = fj.value("field_id", "field_" + std::to_string(idx++));
            f.field = fj.value("field", "");
            f.category = fj.value("category",
                (f.field.find("course_") == 0 || f.field == "credits" || f.field == "grade" || f.field == "quality_points")
                ? "course_record" : "identity");
            f.isSemanticValid = true;
            f.final.value = fj.value("value", "");
            f.final.confidence = fj.value("confidence", 1.0f);
            f.term = fj.value("term", "");
            f.status = fj.value("status", "");

            if (fj.contains("resolution") && fj["resolution"].is_object()) {
                f.resolutionMethod = fj["resolution"].value("method", "OCR");
                f.resolutionStatus = resolutionStatusFromString(fj["resolution"].value("status", "UNRESOLVED"));
            }
            if (fj.contains("source") && fj["source"].is_object()) {
                f.original.source = EvidenceSource::fromJson(fj["source"]);
            }
            res.fields.push_back(std::move(f));
        }
    }

    return project(res, doc);
}

FinalizationReport FinalDocumentProjector::buildFinalizationReport(
    const EnhancedEdrResult& enhancedResult,
    const nlohmann::json& finalDocumentJson,
    int64_t executionTimeMs
) {
    FinalizationReport report;
    report.documentId = enhancedResult.documentId;
    report.source = "enhanced_edr";
    report.executionTimeMs = executionTimeMs;
    report.status = "COMPLETED";

    int projected = 0;
    int missing = 0;
    int llmResCount = 0;
    int detResCount = 0;

    auto checkField = [&](const nlohmann::json& obj, const std::string& key) {
        if (obj.contains(key) && !obj[key].is_null()) {
            projected++;
        } else {
            missing++;
        }
    };

    if (finalDocumentJson.contains("institution") && finalDocumentJson["institution"].is_object()) {
        checkField(finalDocumentJson["institution"], "name");
    }
    if (finalDocumentJson.contains("student") && finalDocumentJson["student"].is_object()) {
        checkField(finalDocumentJson["student"], "name");
        checkField(finalDocumentJson["student"], "student_id");
    }
    if (finalDocumentJson.contains("program") && finalDocumentJson["program"].is_object()) {
        checkField(finalDocumentJson["program"], "degree");
        checkField(finalDocumentJson["program"], "major");
    }
    if (finalDocumentJson.contains("summary") && finalDocumentJson["summary"].is_object()) {
        checkField(finalDocumentJson["summary"], "gpa");
        checkField(finalDocumentJson["summary"], "total_credits_earned");
    }

    if (finalDocumentJson.contains("courses") && finalDocumentJson["courses"].is_array()) {
        for (const auto& c : finalDocumentJson["courses"]) {
            checkField(c, "course_code");
            checkField(c, "course_title");
            checkField(c, "credits");
            checkField(c, "grade");

            std::string resMethod = c.value("resolution_method", "OCR");
            if (resMethod == "LLM") llmResCount++;
            else if (resMethod == "DETERMINISTIC") detResCount++;
        }
    }

    report.fieldsProjected = projected;
    report.fieldsMissing = missing;
    report.fieldsWithLlmResolution = llmResCount;
    report.fieldsWithDeterministicResolution = detResCount;
    report.llmUsed = (llmResCount > 0 || enhancedResult.llmResolvedCount > 0);

    if (finalDocumentJson.contains("confidence") && finalDocumentJson["confidence"].is_object()) {
        report.overallConfidence = finalDocumentJson["confidence"].value("overall", 1.0f);
    }

    return report;
}

} // namespace edmo::evidence

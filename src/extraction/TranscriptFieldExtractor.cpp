#include <edmo/evidence/extraction/TranscriptFieldExtractor.hpp>

#include <algorithm>
#include <regex>
#include <set>
#include <sstream>

namespace edmo::evidence {

namespace {

std::string toUpper(std::string_view sv) {
    std::string s(sv);
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return s;
}

bool containsCi(std::string_view hay, std::string_view needle) {
    return toUpper(hay).find(toUpper(needle)) != std::string::npos;
}

std::string_view trimWhitespace(std::string_view sv) noexcept {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }
    return sv;
}

} // namespace

std::vector<EvidenceField> TranscriptFieldExtractor::extractFields(
    const EvidenceDocument& doc
) const {
    std::vector<EvidenceField> fields;
    fields.reserve(64);

    extractIdentityFields(doc, fields);
    extractInstitutionFields(doc, fields);
    extractAcademicFields(doc, fields);
    extractTableRecords(doc, fields);
    extractUnclassifiedFields(doc, fields);

    return fields;
}

void TranscriptFieldExtractor::extractIdentityFields(
    const EvidenceDocument& doc,
    std::vector<EvidenceField>& outFields
) const {
    int idCounter = 1;

    auto findBlock = [&](const std::string& fieldName, const std::string& val) -> const EvidenceBlock* {
        for (const auto& page : doc.pages) {
            for (const auto& block : page.blocks) {
                if (block.type == BlockType::LabelValue) {
                    auto lbl = toUpper(block.label);
                    bool match = false;
                    if (fieldName == "student_name" && (lbl == "NAME" || lbl == "STUDENT NAME" || lbl == "STUDENT" || lbl == "RECORD OF:")) match = true;
                    else if (fieldName == "student_id" && (lbl == "STUDENT ID" || lbl == "ID" || lbl == "ID NUMBER" || lbl == "STUDENT NO")) match = true;
                    else if (fieldName == "birthdate" && (lbl == "BIRTHDATE" || lbl == "DATE OF BIRTH" || lbl == "DOB")) match = true;
                    if (match) {
                        if (val.empty() || block.value == val || block.value.find(val) != std::string::npos || val.find(block.value) != std::string::npos) {
                            return &block;
                        }
                    }
                }
            }
        }
        return nullptr;
    };

    // Check identity map first
    if (auto it = doc.identity.find("student_name"); it != doc.identity.end() && !it->second.empty()) {
        EvidenceField f;
        f.fieldId = "field_student_name_" + std::to_string(idCounter++);
        f.field = "student_name";
        f.category = "identity";
        f.structuralClass = StructuralClass::StudentMetadata;
        f.isSemanticValid = true;
        f.original.value = it->second;
        f.original.confidence = 0.95f;
        f.original.source.page = 1;
        f.original.source.field = "student_name";
        f.semanticRelationship = "VALUE_FOR_LABEL";

        if (const auto* blk = findBlock("student_name", it->second)) {
            f.sourceBlockId = blk->id;
            f.labelBlockId = "label_" + blk->id;
            f.labelText = blk->label.empty() ? "NAME" : blk->label;
            f.original.source = blk->source;
            if (blk->confidence.mean > 0.0f) {
                f.original.confidence = blk->confidence.mean;
                f.original.minConfidence = blk->confidence.minimum;
                f.original.meanConfidence = blk->confidence.mean;
                f.original.maxConfidence = blk->confidence.maximum;
                f.original.lowConfidenceTokenCount = blk->confidence.lowConfidenceCount;
            }
        }

        f.normalization = FieldValueNormalizer::normalize("student_name", it->second, f.labelText.empty() ? "NAME" : f.labelText);
        f.final.value = f.normalization.normalizedValue;
        f.final.confidence = f.original.confidence;
        f.fate = EvidenceFate::Preserved;
        outFields.push_back(std::move(f));
    }

    if (auto it = doc.identity.find("student_id"); it != doc.identity.end() && !it->second.empty()) {
        EvidenceField f;
        f.fieldId = "field_student_id_" + std::to_string(idCounter++);
        f.field = "student_id";
        f.category = "identity";
        f.structuralClass = StructuralClass::StudentMetadata;
        f.isSemanticValid = true;
        f.original.value = it->second;
        f.original.confidence = 0.95f;
        f.original.source.page = 1;
        f.original.source.field = "student_id";
        f.semanticRelationship = "VALUE_FOR_LABEL";

        if (const auto* blk = findBlock("student_id", it->second)) {
            f.sourceBlockId = blk->id;
            f.labelBlockId = "label_" + blk->id;
            f.labelText = blk->label.empty() ? "STUDENT ID" : blk->label;
            f.original.source = blk->source;
            if (blk->confidence.mean > 0.0f) {
                f.original.confidence = blk->confidence.mean;
                f.original.minConfidence = blk->confidence.minimum;
                f.original.meanConfidence = blk->confidence.mean;
                f.original.maxConfidence = blk->confidence.maximum;
                f.original.lowConfidenceTokenCount = blk->confidence.lowConfidenceCount;
            }
        }

        f.normalization = FieldValueNormalizer::normalize("student_id", it->second, f.labelText.empty() ? "STUDENT ID" : f.labelText);
        f.final.value = f.normalization.normalizedValue;
        f.final.confidence = f.original.confidence;
        f.fate = EvidenceFate::Preserved;
        outFields.push_back(std::move(f));
    }

    if (auto it = doc.identity.find("birthdate"); it != doc.identity.end() && !it->second.empty()) {
        EvidenceField f;
        f.fieldId = "field_birthdate_" + std::to_string(idCounter++);
        f.field = "birthdate";
        f.category = "identity";
        f.structuralClass = StructuralClass::StudentMetadata;
        f.isSemanticValid = true;
        f.original.value = it->second;
        f.original.confidence = 0.90f;
        f.original.source.page = 1;
        f.original.source.field = "birthdate";
        f.semanticRelationship = "VALUE_FOR_LABEL";

        if (const auto* blk = findBlock("birthdate", it->second)) {
            f.sourceBlockId = blk->id;
            f.labelBlockId = "label_" + blk->id;
            f.labelText = blk->label.empty() ? "BIRTHDATE" : blk->label;
            f.original.source = blk->source;
            if (blk->confidence.mean > 0.0f) {
                f.original.confidence = blk->confidence.mean;
                f.original.minConfidence = blk->confidence.minimum;
                f.original.meanConfidence = blk->confidence.mean;
                f.original.maxConfidence = blk->confidence.maximum;
                f.original.lowConfidenceTokenCount = blk->confidence.lowConfidenceCount;
            }
        }

        f.normalization = FieldValueNormalizer::normalize("birthdate", it->second, f.labelText.empty() ? "BIRTHDATE" : f.labelText);
        f.final.value = f.normalization.normalizedValue;
        f.final.confidence = f.original.confidence;
        f.fate = EvidenceFate::Preserved;
        outFields.push_back(std::move(f));
    }

    // Also scan blocks for explicit label_value blocks (e.g. NAME, STUDENT ID, BIRTHDATE)
    for (const auto& page : doc.pages) {
        for (const auto& block : page.blocks) {
            if (block.type == BlockType::LabelValue && !block.value.empty()) {
                const auto lbl = toUpper(block.label);
                std::string fieldName;
                std::string category = "identity";

                if (lbl == "NAME" || lbl == "STUDENT NAME" || lbl == "STUDENT" || lbl == "RECORD OF:") {
                    fieldName = "student_name";
                } else if (lbl == "STUDENT ID" || lbl == "ID" || lbl == "ID NUMBER" || lbl == "STUDENT NO") {
                    fieldName = "student_id";
                } else if (lbl == "BIRTHDATE" || lbl == "DATE OF BIRTH" || lbl == "DOB") {
                    fieldName = "birthdate";
                } else if (lbl == "DEPARTMENT" || lbl == "FACULTY") {
                    fieldName = "department";
                    category = "academic";
                } else if (containsCi(lbl, "MAJOR") || lbl == "PROGRAM" || containsCi(lbl, "FIELD OF STUDY") || containsCi(lbl, "CONCENTRATION")) {
                    fieldName = "major";
                    category = "academic";
                } else if (containsCi(lbl, "DEGREE")) {
                    fieldName = "degree";
                    category = "academic";
                }

                if (!fieldName.empty()) {
                    auto itField = std::find_if(outFields.begin(), outFields.end(), [&](const EvidenceField& ef) {
                        return ef.field == fieldName;
                    });

                    if (itField != outFields.end()) {
                        // Enrich existing field if missing source block or bbox
                        if (itField->sourceBlockId.empty()) itField->sourceBlockId = block.id;
                        if (itField->labelBlockId.empty()) itField->labelBlockId = "label_" + block.id;
                        if (itField->labelText.empty()) itField->labelText = block.label;
                        if (itField->semanticRelationship.empty()) itField->semanticRelationship = "VALUE_FOR_LABEL";
                        if (itField->original.source.bbox.empty()) itField->original.source = block.source;
                        if (itField->original.value.empty()) itField->original.value = block.value;
                        if (itField->final.value.empty()) itField->final.value = block.value;
                    } else {
                        EvidenceField f;
                        f.fieldId = "field_" + fieldName + "_" + std::to_string(idCounter++);
                        f.field = fieldName;
                        f.category = category;
                        f.sourceBlockId = block.id;
                        f.labelBlockId = "label_" + block.id;
                        f.labelText = block.label;
                        f.semanticRelationship = "VALUE_FOR_LABEL";
                        f.structuralClass = StructuralClass::StudentMetadata;
                        f.isSemanticValid = true;
                        f.original.value = block.value;
                        f.original.confidence = block.confidence.mean > 0.0f ? block.confidence.mean : 0.95f;
                        f.original.minConfidence = block.confidence.minimum;
                        f.original.meanConfidence = block.confidence.mean;
                        f.original.maxConfidence = block.confidence.maximum;
                        f.original.lowConfidenceTokenCount = block.confidence.lowConfidenceCount;
                        f.original.source = block.source;
                        f.normalization = FieldValueNormalizer::normalize(fieldName, block.value, block.label);
                        f.final.value = f.normalization.normalizedValue;
                        f.final.confidence = f.original.confidence;
                        f.fate = EvidenceFate::Preserved;
                        outFields.push_back(std::move(f));
                    }
                }
            }
        }
    }
}

void TranscriptFieldExtractor::extractInstitutionFields(
    const EvidenceDocument& doc,
    std::vector<EvidenceField>& outFields
) const {
    int idCounter = 1;
    if (auto it = doc.institution.find("name"); it != doc.institution.end() && !it->second.empty()) {
        EvidenceField f;
        f.fieldId = "field_institution_name_" + std::to_string(idCounter++);
        f.field = "institution_name";
        f.category = "institution";
        f.structuralClass = StructuralClass::InstitutionData;
        f.isSemanticValid = true;
        f.original.value = it->second;
        f.original.confidence = 0.95f;
        f.original.source.page = 1;
        f.semanticRelationship = "INSTITUTION_HEADER";

        // Find matching block
        for (const auto& page : doc.pages) {
            for (const auto& block : page.blocks) {
                if (containsCi(block.text, it->second) || (block.type == BlockType::Header && (containsCi(block.text, "COLLEGE") || containsCi(block.text, "UNIVERSITY") || containsCi(block.text, "OXFORD")))) {
                    f.sourceBlockId = block.id;
                    f.labelBlockId = "label_" + block.id;
                    f.labelText = block.label.empty() ? "INSTITUTION" : block.label;
                    f.original.source = block.source;
                    if (block.confidence.mean > 0.0f) {
                        f.original.confidence = block.confidence.mean;
                        f.original.minConfidence = block.confidence.minimum;
                        f.original.meanConfidence = block.confidence.mean;
                        f.original.maxConfidence = block.confidence.maximum;
                        f.original.lowConfidenceTokenCount = block.confidence.lowConfidenceCount;
                    }
                    break;
                }
            }
            if (!f.sourceBlockId.empty()) break;
        }

        f.normalization = FieldValueNormalizer::normalize("institution_name", it->second, "");
        f.final.value = f.normalization.normalizedValue;
        f.final.confidence = f.original.confidence;
        f.fate = EvidenceFate::Preserved;
        outFields.push_back(std::move(f));
    }
}

void TranscriptFieldExtractor::extractAcademicFields(
    const EvidenceDocument& doc,
    std::vector<EvidenceField>& outFields
) const {
    int idCounter = 1;
    for (const auto& [k, v] : doc.academic) {
        if (v.empty()) continue;

        // Check if already extracted
        auto itExisting = std::find_if(outFields.begin(), outFields.end(), [&](const EvidenceField& ef) {
            return ef.field == k;
        });

        if (itExisting != outFields.end()) {
            if (itExisting->final.value.empty()) itExisting->final.value = v;
            continue;
        }

        EvidenceField f;
        f.fieldId = "field_academic_" + k + "_" + std::to_string(idCounter++);
        f.field = k;
        f.category = "academic";
        f.structuralClass = StructuralClass::StudentMetadata;
        f.isSemanticValid = true;
        f.original.value = v;
        f.original.confidence = 0.95f;
        f.original.source.page = 1;
        f.semanticRelationship = "VALUE_FOR_LABEL";

        // Find matching block in doc.pages
        for (const auto& page : doc.pages) {
            for (const auto& block : page.blocks) {
                if (block.type == BlockType::LabelValue) {
                    auto lbl = toUpper(block.label);
                    bool match = false;
                    if (k == "major" && (containsCi(lbl, "MAJOR") || containsCi(lbl, "PROGRAM") || containsCi(lbl, "FIELD OF STUDY") || containsCi(lbl, "CONCENTRATION"))) match = true;
                    else if (k == "degree" && containsCi(lbl, "DEGREE")) match = true;
                    else if (k == "school" && (containsCi(lbl, "SCHOOL") || containsCi(lbl, "COLLEGE"))) match = true;
                    else if (k == "department" && (containsCi(lbl, "DEPARTMENT") || containsCi(lbl, "FACULTY"))) match = true;
                    else if (k == "minor" && containsCi(lbl, "MINOR")) match = true;

                    if (match && (block.value == v || block.value.find(v) != std::string::npos || v.find(block.value) != std::string::npos)) {
                        f.sourceBlockId = block.id;
                        f.labelBlockId = "label_" + block.id;
                        f.labelText = block.label;
                        f.original.source = block.source;
                        if (block.confidence.mean > 0.0f) {
                            f.original.confidence = block.confidence.mean;
                            f.original.minConfidence = block.confidence.minimum;
                            f.original.meanConfidence = block.confidence.mean;
                            f.original.maxConfidence = block.confidence.maximum;
                            f.original.lowConfidenceTokenCount = block.confidence.lowConfidenceCount;
                        }
                        break;
                    }
                }
            }
            if (!f.sourceBlockId.empty()) break;
        }

        f.normalization = FieldValueNormalizer::normalize(k, v, f.labelText);
        f.final.value = f.normalization.normalizedValue;
        f.final.confidence = f.original.confidence;
        f.fate = EvidenceFate::Preserved;
        outFields.push_back(std::move(f));
    }
}

void TranscriptFieldExtractor::extractTableRecords(
    const EvidenceDocument& doc,
    std::vector<EvidenceField>& outFields
) const {
    int recordIdx = 1;
    int structIdx = 1;
    std::string currentTerm;

    for (const auto& tbl : doc.tables) {
        std::set<size_t> consumedRows;
        for (size_t r = 0; r < tbl.rows.size(); ++r) {
            if (consumedRows.count(r)) continue;
            const auto& row = tbl.rows[r];
            if (row.empty()) continue;

            // Combine cell text for structural analysis
            std::string rowText;
            for (size_t c = 0; c < row.cells.size(); ++c) {
                if (c > 0) rowText += " ";
                rowText += row.cells[c].text;
            }
            if (rowText.empty() && !row.canonicalFields.empty()) {
                if (auto it = row.canonicalFields.find("course_title"); it != row.canonicalFields.end()) rowText += it->second;
            }

            auto trimmedRowText = trimWhitespace(rowText);

            // 1. Table Separator
            if (PlaceholderDetector::isSeparatorLine(trimmedRowText)) {
                EvidenceField f;
                f.fieldId = "struct_separator_" + std::to_string(structIdx++);
                f.field = "table_separator";
                f.category = "structure";
                f.structuralClass = StructuralClass::TableSeparator;
                f.original.value = std::string(trimmedRowText);
                f.original.confidence = 0.0f;
                f.original.source = row.source;
                f.final.value = std::string(trimmedRowText);
                f.final.confidence = 0.0f;
                f.isSemanticValid = false;
                f.llmEligible = false;
                f.llmReason = "TABLE_SEPARATOR";
                f.fate = EvidenceFate::Noise;
                outFields.push_back(std::move(f));
                continue;
            }

            // 2. Term Header (e.g. "----------------- Fall 2024 -----------------" or "Fall 2024")
            if (classifier_.isTermHeader(trimmedRowText)) {
                currentTerm = classifier_.extractTermName(trimmedRowText);
                EvidenceField f;
                f.fieldId = "struct_term_" + std::to_string(structIdx++);
                f.field = "term_header";
                f.category = "structure";
                f.structuralClass = StructuralClass::TermHeader;
                f.term = currentTerm;
                f.original.value = std::string(trimmedRowText);
                f.original.confidence = 0.95f;
                f.original.source = row.source;
                f.final.value = currentTerm;
                f.final.confidence = 0.95f;
                f.isSemanticValid = false;
                f.llmEligible = false;
                f.llmReason = "TERM_HEADER";
                f.fate = EvidenceFate::Structured;
                outFields.push_back(std::move(f));
                continue;
            }

            // 3. Table Column Header
            if (row.isHeader || classifier_.isTableHeader(trimmedRowText)) {
                EvidenceField f;
                f.fieldId = "struct_header_" + std::to_string(structIdx++);
                f.field = "table_header";
                f.category = "structure";
                f.structuralClass = StructuralClass::TableHeader;
                f.original.value = std::string(trimmedRowText);
                f.original.confidence = 0.95f;
                f.original.source = row.source;
                f.final.value = std::string(trimmedRowText);
                f.final.confidence = 0.95f;
                f.isSemanticValid = false;
                f.llmEligible = false;
                f.llmReason = "TABLE_HEADER";
                f.fate = EvidenceFate::Structured;
                outFields.push_back(std::move(f));
                continue;
            }

            // 4. Course Status Header (e.g. "In Progress:")
            if (classifier_.isCourseStatusHeader(trimmedRowText)) {
                EvidenceField f;
                f.fieldId = "struct_status_" + std::to_string(structIdx++);
                f.field = "course_status_header";
                f.category = "structure";
                f.structuralClass = StructuralClass::CourseStatusHeader;
                f.original.value = std::string(trimmedRowText);
                f.original.confidence = 0.95f;
                f.original.source = row.source;
                f.final.value = std::string(trimmedRowText);
                f.final.confidence = 0.95f;
                f.isSemanticValid = false;
                f.llmEligible = false;
                f.llmReason = "COURSE_STATUS_HEADER";
                f.fate = EvidenceFate::Structured;
                outFields.push_back(std::move(f));
                continue;
            }

            // 5. Academic Distinction (e.g. "First Honors")
            if (classifier_.isAcademicDistinction(trimmedRowText)) {
                EvidenceField f;
                f.fieldId = "field_distinction_" + std::to_string(structIdx++);
                f.field = "academic_distinction";
                f.category = "academic";
                f.structuralClass = StructuralClass::AcademicDistinction;
                f.term = currentTerm;
                f.original.value = std::string(trimmedRowText);
                f.original.confidence = 0.95f;
                f.original.source = row.source;
                f.final.value = std::string(trimmedRowText);
                f.final.confidence = 0.95f;
                f.isSemanticValid = true;
                f.llmEligible = false;
                f.fate = EvidenceFate::Structured;
                outFields.push_back(std::move(f));
                continue;
            }

            // 6. Summary Structure (Header / Summary Row)
            if (classifier_.isSummaryStructure(trimmedRowText)) {
                bool isRow = (toUpper(trimmedRowText).find("CURRENT ") == 0 ||
                              toUpper(trimmedRowText).find("CUMULATIVE ") == 0 ||
                              toUpper(trimmedRowText).find("TOTAL ") == 0);
                EvidenceField f;
                f.fieldId = "summary_" + std::string(isRow ? "row_" : "hdr_") + std::to_string(structIdx++);
                f.field = isRow ? "summary_row" : "summary_header";
                f.category = "summary";
                f.structuralClass = isRow ? StructuralClass::SummaryRow : StructuralClass::SummaryHeader;
                f.term = currentTerm;
                f.original.value = std::string(trimmedRowText);
                f.original.confidence = 0.95f;
                f.original.source = row.source;
                f.final.value = std::string(trimmedRowText);
                f.final.confidence = 0.95f;
                f.isSemanticValid = false;
                f.llmEligible = false;
                f.llmReason = "SUMMARY_STRUCTURE";
                f.fate = EvidenceFate::Structured;
                outFields.push_back(std::move(f));
                continue;
            }

            // 7. Parse as Course Row
            auto parsed = courseRowDetector_.parseRow(row, currentTerm);
            if (parsed.isValid) {
                // If grade is missing and course is not in-progress, attempt structural/layout association
                if (parsed.grade.empty() && parsed.status != "IN_PROGRESS") {
                    static const std::regex validGradeRegex(R"(^(?:[A-F][+-]?|[A-F][a-z]?|P|S|CR|NC|W|I|AUDIT)$)");
                    auto sanitizeGrade = [](std::string g) -> std::string {
                        if (g.size() == 2 && std::isalpha(static_cast<unsigned char>(g[0])) && std::islower(static_cast<unsigned char>(g[1]))) {
                            return std::string(1, static_cast<char>(std::toupper(g[0])));
                        }
                        return g;
                    };

                    // 1. Check cells of the CURRENT row first
                    for (const auto& cell : row.cells) {
                        std::string cText = std::string(trimWhitespace(cell.text));
                        std::string canUpper = toUpper(cell.canonicalColumnName);
                        std::string colUpper = toUpper(cell.columnName);
                        if ((canUpper == "GRADE" || colUpper == "GRADE" || colUpper == "GRD" || cell.fate == EvidenceFate::Structured) &&
                            std::regex_match(cText, validGradeRegex)) {
                            parsed.grade = sanitizeGrade(cText);
                            parsed.gradeConfidence = cell.confidence > 0.0f ? cell.confidence : 0.95f;
                            parsed.nearbyEvidence.push_back("Grade: " + parsed.grade);
                            break;
                        }
                    }

                    int yr = row.source.bbox.y;
                    int hr = row.source.bbox.height > 0 ? row.source.bbox.height : 30;

                    // A. Check subsequent nearby rows in this table with vertical Y-overlap
                    if (parsed.grade.empty()) {
                        for (size_t k = r + 1; k < std::min(r + 4, tbl.rows.size()); ++k) {
                        if (consumedRows.count(k)) continue;
                        const auto& otherRow = tbl.rows[k];
                        if (otherRow.source.page != row.source.page) continue;

                        std::string candGrade;
                        if (auto it = otherRow.canonicalFields.find("grade"); it != otherRow.canonicalFields.end() && !it->second.empty()) {
                            candGrade = it->second;
                        } else if (auto it2 = otherRow.canonicalFields.find("course_code"); it2 != otherRow.canonicalFields.end() && std::regex_match(it2->second, validGradeRegex)) {
                            candGrade = it2->second;
                        } else if (!otherRow.cells.empty() && std::regex_match(std::string(trimWhitespace(otherRow.cells[0].text)), validGradeRegex)) {
                            candGrade = std::string(trimWhitespace(otherRow.cells[0].text));
                        }

                        if (!candGrade.empty() && std::regex_match(candGrade, validGradeRegex)) {
                            int yk = otherRow.source.bbox.y;
                            int hk = otherRow.source.bbox.height > 0 ? otherRow.source.bbox.height : 20;
                            int dy = std::abs((yk + hk / 2) - (yr + hr / 2));
                            if (dy <= std::max(hr, 35) && (otherRow.source.bbox.x >= row.source.bbox.x + 200 || otherRow.source.bbox.x >= 1000)) {
                                parsed.grade = sanitizeGrade(candGrade);
                                parsed.gradeConfidence = otherRow.confidence > 0.0f ? otherRow.confidence : 0.95f;
                                parsed.nearbyEvidence.push_back("Grade: " + parsed.grade);
                                consumedRows.insert(k);
                                break;
                            }
                        }
                    }
                    }

                    // B. If still empty, check OCR blocks on the same page with bounding box alignment
                    if (parsed.grade.empty()) {
                        for (const auto& p : doc.pages) {
                            if (p.pageNumber != row.source.page) continue;
                            for (const auto& blk : p.blocks) {
                                auto trimmedBlk = std::string(trimWhitespace(blk.text));
                                if (std::regex_match(trimmedBlk, validGradeRegex)) {
                                    int yb = blk.source.bbox.y;
                                    int hb = blk.source.bbox.height > 0 ? blk.source.bbox.height : 20;
                                    int dy = std::abs((yb + hb / 2) - (yr + hr / 2));
                                    if (dy <= std::max(hr, 35) && (blk.source.bbox.x >= row.source.bbox.x + 200 || blk.source.bbox.x >= 1000)) {
                                        parsed.grade = sanitizeGrade(trimmedBlk);
                                        parsed.gradeConfidence = blk.confidence.mean > 0.0f ? blk.confidence.mean : 0.95f;
                                        parsed.nearbyEvidence.push_back("Grade: " + parsed.grade);
                                        break;
                                    }
                                }
                            }
                            if (!parsed.grade.empty()) break;
                        }
                    }
                }

                // Course Code
                if (!parsed.courseCode.empty()) {
                    EvidenceField f;
                    f.fieldId = "field_course_code_" + std::to_string(recordIdx);
                    f.field = "course_code";
                    f.category = "course_record";
                    f.sourceBlockId = "block_src_course_code";
                    f.labelBlockId = tbl.id.empty() ? ("table_p" + std::to_string(row.source.page) + "_1") : tbl.id;
                    f.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
                    f.structuralClass = StructuralClass::CourseRow;
                    f.term = currentTerm;
                    f.status = parsed.status;
                    f.isSemanticValid = true;
                    f.original.value = parsed.courseCode;
                    f.original.confidence = parsed.codeConfidence;
                    f.original.source = row.source;
                    f.original.source.field = "course_code";
                    f.final.value = parsed.courseCode;
                    f.final.confidence = parsed.codeConfidence;
                    f.nearbyEvidence = parsed.nearbyEvidence;
                    f.fate = EvidenceFate::Structured;
                    outFields.push_back(std::move(f));
                }

                // Course Title
                if (!parsed.courseTitle.empty()) {
                    EvidenceField f;
                    f.fieldId = "field_course_title_" + std::to_string(recordIdx);
                    f.field = "course_title";
                    f.category = "course_record";
                    f.sourceBlockId = "block_src_course_title";
                    f.labelBlockId = tbl.id.empty() ? ("table_p" + std::to_string(row.source.page) + "_1") : tbl.id;
                    f.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
                    f.structuralClass = StructuralClass::CourseRow;
                    f.term = currentTerm;
                    f.status = parsed.status;
                    f.isSemanticValid = true;
                    f.original.value = parsed.courseTitle;
                    f.original.confidence = parsed.titleConfidence;
                    f.original.source = row.source;
                    f.original.source.field = "course_title";
                    f.final.value = parsed.courseTitle;
                    f.final.confidence = parsed.titleConfidence;
                    f.nearbyEvidence = parsed.nearbyEvidence;
                    f.fate = EvidenceFate::Structured;
                    outFields.push_back(std::move(f));
                }

                // Credits
                if (!parsed.credits.empty()) {
                    EvidenceField f;
                    f.fieldId = "field_credits_" + std::to_string(recordIdx);
                    f.field = "credits";
                    f.category = "course_record";
                    f.sourceBlockId = "block_src_credits";
                    f.labelBlockId = tbl.id.empty() ? ("table_p" + std::to_string(row.source.page) + "_1") : tbl.id;
                    f.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
                    f.structuralClass = StructuralClass::CourseRow;
                    f.term = currentTerm;
                    f.status = parsed.status;
                    f.isSemanticValid = true;
                    f.original.value = parsed.credits;
                    f.original.confidence = parsed.creditsConfidence;
                    f.original.source = row.source;
                    f.original.source.field = "credits";
                    f.final.value = parsed.credits;
                    f.final.confidence = parsed.creditsConfidence;
                    f.nearbyEvidence = parsed.nearbyEvidence;
                    f.fate = EvidenceFate::Structured;
                    outFields.push_back(std::move(f));
                }

                // Grade (only if completed and not empty)
                if (!parsed.grade.empty() && parsed.status != "IN_PROGRESS") {
                    EvidenceField f;
                    f.fieldId = "field_grade_" + std::to_string(recordIdx);
                    f.field = "grade";
                    f.category = "course_record";
                    f.sourceBlockId = "block_src_grade";
                    f.labelBlockId = tbl.id.empty() ? ("table_p" + std::to_string(row.source.page) + "_1") : tbl.id;
                    f.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
                    f.structuralClass = StructuralClass::CourseRow;
                    f.term = currentTerm;
                    f.status = parsed.status;
                    f.isSemanticValid = true;
                    f.original.value = parsed.grade;
                    EvidenceSource gradeSource = row.source;
                    gradeSource.field = "grade";
                    for (const auto& cell : row.cells) {
                        std::string canUpper = toUpper(cell.canonicalColumnName);
                        std::string colUpper = toUpper(cell.columnName);
                        if ((canUpper == "GRADE" || colUpper == "GRADE" || cell.text == parsed.grade) && !cell.source.empty()) {
                            gradeSource = cell.source;
                            gradeSource.field = "grade";
                            break;
                        }
                    }
                    f.original.source = gradeSource;
                    f.final.value = parsed.grade;
                    f.final.confidence = parsed.gradeConfidence;
                    f.nearbyEvidence = parsed.nearbyEvidence;
                    f.fate = EvidenceFate::Structured;
                    outFields.push_back(std::move(f));
                }

                // Quality Points
                if (!parsed.qualityPoints.empty()) {
                    EvidenceField f;
                    f.fieldId = "field_quality_points_" + std::to_string(recordIdx);
                    f.field = "quality_points";
                    f.category = "course_record";
                    f.sourceBlockId = "block_src_quality_points";
                    f.labelBlockId = tbl.id.empty() ? ("table_p" + std::to_string(row.source.page) + "_1") : tbl.id;
                    f.semanticRelationship = "TABLE_COLUMN_ASSOCIATION";
                    f.structuralClass = StructuralClass::CourseRow;
                    f.term = currentTerm;
                    f.status = parsed.status;
                    f.isSemanticValid = true;
                    f.original.value = parsed.qualityPoints;
                    f.original.confidence = parsed.confidence;
                    f.original.source = row.source;
                    f.original.source.field = "quality_points";
                    f.final.value = parsed.qualityPoints;
                    f.final.confidence = parsed.confidence;
                    f.nearbyEvidence = parsed.nearbyEvidence;
                    f.fate = EvidenceFate::Structured;
                    outFields.push_back(std::move(f));
                }

                recordIdx++;
                continue;
            }

            // Fallback for non-course table rows: preserve text without falsely declaring course code/title
            EvidenceField f;
            f.fieldId = "row_generic_" + std::to_string(structIdx++);
            f.field = "table_row";
            f.category = "table";
            f.structuralClass = StructuralClass::Data;
            f.original.value = trimmedRowText.empty() ? rowText : std::string(trimmedRowText);
            f.original.confidence = row.confidence;
            f.original.source = row.source;
            f.final.value = f.original.value;
            f.final.confidence = row.confidence;
            f.isSemanticValid = false;
            f.llmEligible = false;
            f.fate = EvidenceFate::Structured;
            outFields.push_back(std::move(f));
        }
    }
}

void TranscriptFieldExtractor::extractUnclassifiedFields(
    const EvidenceDocument& doc,
    std::vector<EvidenceField>& outFields
) const {
    int unclassifiedIdx = 1;
    for (const auto& uc : doc.unclassifiedContent) {
        if (uc.empty()) continue;
        auto trimmed = trimWhitespace(uc);

        // Check if unclassified is a separator
        if (PlaceholderDetector::isSeparatorLine(trimmed)) {
            EvidenceField f;
            f.fieldId = "unclass_sep_" + std::to_string(unclassifiedIdx++);
            f.field = "separator";
            f.category = "structure";
            f.structuralClass = StructuralClass::TableSeparator;
            f.original.value = std::string(trimmed);
            f.original.confidence = 0.0f;
            f.original.source.page = 1;
            f.final.value = std::string(trimmed);
            f.final.confidence = 0.0f;
            f.isSemanticValid = false;
            f.llmEligible = false;
            f.llmReason = "SEPARATOR_LINE";
            f.fate = EvidenceFate::Noise;
            outFields.push_back(std::move(f));
            continue;
        }

        // Check if unclassified is placeholder
        if (PlaceholderDetector::isPlaceholder(trimmed)) {
            EvidenceField f;
            f.fieldId = "unclass_ph_" + std::to_string(unclassifiedIdx++);
            f.field = "placeholder";
            f.category = "structure";
            f.structuralClass = StructuralClass::Placeholder;
            f.original.value = std::string(trimmed);
            f.original.confidence = 0.0f;
            f.original.source.page = 1;
            f.final.value = std::string(trimmed);
            f.final.confidence = 0.0f;
            f.isSemanticValid = false;
            f.llmEligible = false;
            f.llmReason = "PLACEHOLDER";
            f.fate = EvidenceFate::Noise;
            outFields.push_back(std::move(f));
            continue;
        }

        // Check if term header
        if (classifier_.isTermHeader(trimmed)) {
            std::string tName = classifier_.extractTermName(trimmed);
            EvidenceField f;
            f.fieldId = "unclass_term_" + std::to_string(unclassifiedIdx++);
            f.field = "term_header";
            f.category = "structure";
            f.structuralClass = StructuralClass::TermHeader;
            f.term = tName;
            f.original.value = std::string(trimmed);
            f.original.confidence = 0.95f;
            f.original.source.page = 1;
            f.final.value = tName;
            f.final.confidence = 0.95f;
            f.isSemanticValid = false;
            f.llmEligible = false;
            f.fate = EvidenceFate::Structured;
            outFields.push_back(std::move(f));
            continue;
        }

        // Check if course row
        if (courseRowDetector_.isCourseRow(trimmed)) {
            auto parsed = courseRowDetector_.parseText(trimmed, 0.95f);
            if (parsed.isValid) {
                if (!parsed.courseCode.empty()) {
                    EvidenceField f;
                    f.fieldId = "unclass_code_" + std::to_string(unclassifiedIdx);
                    f.field = "course_code";
                    f.category = "course_record";
                    f.structuralClass = StructuralClass::CourseRow;
                    f.isSemanticValid = true;
                    f.original.value = parsed.courseCode;
                    f.original.confidence = parsed.codeConfidence;
                    f.original.source.page = 1;
                    f.final.value = parsed.courseCode;
                    f.final.confidence = parsed.codeConfidence;
                    f.nearbyEvidence = parsed.nearbyEvidence;
                    f.fate = EvidenceFate::Structured;
                    outFields.push_back(std::move(f));
                }
                if (!parsed.courseTitle.empty()) {
                    EvidenceField f;
                    f.fieldId = "unclass_title_" + std::to_string(unclassifiedIdx);
                    f.field = "course_title";
                    f.category = "course_record";
                    f.structuralClass = StructuralClass::CourseRow;
                    f.isSemanticValid = true;
                    f.original.value = parsed.courseTitle;
                    f.original.confidence = parsed.titleConfidence;
                    f.original.source.page = 1;
                    f.final.value = parsed.courseTitle;
                    f.final.confidence = parsed.titleConfidence;
                    f.nearbyEvidence = parsed.nearbyEvidence;
                    f.fate = EvidenceFate::Structured;
                    outFields.push_back(std::move(f));
                }
                if (!parsed.credits.empty()) {
                    EvidenceField f;
                    f.fieldId = "unclass_crd_" + std::to_string(unclassifiedIdx);
                    f.field = "credits";
                    f.category = "course_record";
                    f.structuralClass = StructuralClass::CourseRow;
                    f.isSemanticValid = true;
                    f.original.value = parsed.credits;
                    f.original.confidence = parsed.creditsConfidence;
                    f.original.source.page = 1;
                    f.final.value = parsed.credits;
                    f.final.confidence = parsed.creditsConfidence;
                    f.nearbyEvidence = parsed.nearbyEvidence;
                    f.fate = EvidenceFate::Structured;
                    outFields.push_back(std::move(f));
                }
                if (!parsed.grade.empty() && parsed.status != "IN_PROGRESS") {
                    EvidenceField f;
                    f.fieldId = "unclass_grd_" + std::to_string(unclassifiedIdx);
                    f.field = "grade";
                    f.category = "course_record";
                    f.structuralClass = StructuralClass::CourseRow;
                    f.isSemanticValid = true;
                    f.original.value = parsed.grade;
                    f.original.confidence = parsed.gradeConfidence;
                    f.original.source.page = 1;
                    f.final.value = parsed.grade;
                    f.final.confidence = parsed.gradeConfidence;
                    f.nearbyEvidence = parsed.nearbyEvidence;
                    f.fate = EvidenceFate::Structured;
                    outFields.push_back(std::move(f));
                }
                unclassifiedIdx++;
                continue;
            }
        }

        // Generic unclassified preserved
        EvidenceField f;
        f.fieldId = "field_unclassified_" + std::to_string(unclassifiedIdx++);
        f.field = "unclassified";
        f.category = "unclassified";
        f.structuralClass = classifier_.classifyText(trimmed);
        f.isSemanticValid = false;
        f.llmEligible = false;
        f.original.value = std::string(trimmed);
        f.original.confidence = 0.85f;
        f.original.source.page = 1;
        f.final.value = std::string(trimmed);
        f.final.confidence = 0.85f;
        f.fate = EvidenceFate::Unclassified;
        outFields.push_back(std::move(f));
    }
}

} // namespace edmo::evidence

#include <ocr/evidence/resolution/ResolutionAnalyzer.hpp>
#include <ocr/evidence/confidence/ConfidenceComparator.hpp>

#include <algorithm>
#include <cctype>
#include <regex>
#include <set>
#include <sstream>

namespace ocr::evidence {

namespace {

std::string trim(std::string_view s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string_view::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r");
    return std::string(s.substr(start, end - start + 1));
}

std::string toUpper(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return out;
}

std::vector<std::string> tokenizeWords(std::string_view s) {
    std::vector<std::string> tokens;
    std::string current;
    for (char c : s) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            current.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        } else if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

bool isNumericString(std::string_view s) {
    auto trimmed = trim(s);
    if (trimmed.empty()) return false;
    bool hasDot = false;
    for (size_t i = 0; i < trimmed.size(); ++i) {
        char c = trimmed[i];
        if (c == '.') {
            if (hasDot) return false;
            hasDot = true;
        } else if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

std::string extractIdSuffix(const std::string& fieldId) {
    auto pos = fieldId.rfind('_');
    if (pos != std::string::npos && pos + 1 < fieldId.size()) {
        return fieldId.substr(pos + 1);
    }
    return fieldId;
}

} // namespace

bool ResolutionAnalyzer::isSupportedLlmField(const std::string& fieldName) const noexcept {
    static const std::set<std::string> supported = {
        "course_title", "course_code", "student_name", "student_id",
        "institution_name", "degree", "major", "minor", "department",
        "school", "grade", "credits", "quality_points", "birthdate", "gpa",
        "cumulative_gpa", "term_gpa", "total_credits", "total_credits_earned"
    };
    return supported.count(fieldName) > 0;
}

bool ResolutionAnalyzer::hasDuplicatedOcrTokens(const std::string& val) const noexcept {
    auto tokens = tokenizeWords(val);
    if (tokens.size() < 2) return false;

    // Check for adjacent duplicate tokens: e.g. "Sophia Sophia"
    for (size_t i = 0; i + 1 < tokens.size(); ++i) {
        if (tokens[i].size() >= 3 && tokens[i] == tokens[i + 1]) {
            return true;
        }
    }

    // Check for doubled phrases: e.g. "Sophia Jones Sophia Jones"
    if (tokens.size() >= 4 && tokens.size() % 2 == 0) {
        size_t half = tokens.size() / 2;
        bool isDoubled = true;
        for (size_t i = 0; i < half; ++i) {
            if (tokens[i] != tokens[i + half]) {
                isDoubled = false;
                break;
            }
        }
        if (isDoubled) return true;
    }

    return false;
}

bool ResolutionAnalyzer::hasMalformedOcrCharacters(const std::string& val) const noexcept {
    if (val.empty()) return false;

    int nonAsciiOrNoise = 0;
    int letterOrDigit = 0;

    for (char c : val) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc) || c == ' ' || c == '-' || c == '&' || c == '/' || c == '.' || c == ',' || c == ':' || c == '(' || c == ')') {
            letterOrDigit++;
        } else {
            nonAsciiOrNoise++;
        }
    }

    // Excessive noise glyphs (> 20% noise or 3+ garbage symbols)
    if (nonAsciiOrNoise >= 3 || (val.size() > 5 && nonAsciiOrNoise * 4 > letterOrDigit)) {
        return true;
    }

    // Check for OCR digit-in-word confusion: e.g. "MATHEMAT1CS", "C0MPUTING", "HIST0RY"
    static const std::regex digitInWordRegex(R"(\b[A-Za-z]{2,}[0-9]+[A-Za-z]*\b|\b[A-Za-z]*[0-9]+[A-Za-z]{2,}\b)");
    if (std::regex_search(val, digitInWordRegex)) {
        return true;
    }

    return false;
}

bool ResolutionAnalyzer::hasConflictingEvidence(
    const EvidenceField& field,
    const std::vector<EvidenceField>& allFields
) const noexcept {
    if (!field.conflictingCandidates.empty()) return true;

    // Check if another field with the same canonical name exists on the same page with a different value
    for (const auto& other : allFields) {
        if (&other == &field) continue;
        if (!other.isSemanticValid || isStructuralOrNonSemantic(other.structuralClass)) continue;

        if (other.field == field.field &&
            other.original.source.page == field.original.source.page &&
            !other.final.value.empty() &&
            !field.final.value.empty() &&
            field.category != "course_record") {
            
            std::string upperA = toUpper(field.final.value);
            std::string upperB = toUpper(other.final.value);
            if (upperA != upperB && upperA.find(upperB) == std::string::npos && upperB.find(upperA) == std::string::npos) {
                return true;
            }
        }
    }

    return false;
}

bool ResolutionAnalyzer::isStrongTableRowEvidence(
    const EvidenceField& field,
    const EvidenceDocument& doc,
    const std::vector<EvidenceField>& allFields
) const noexcept {
    // 1. Must be in a course_record category or have CourseRow structural class
    if (field.category != "course_record" &&
        field.structuralClass != StructuralClass::CourseRow &&
        field.semanticRelationship != "TABLE_COLUMN_ASSOCIATION") {
        return false;
    }

    // 2. Must not be empty
    if (field.final.value.empty() && field.original.value.empty()) {
        return false;
    }

    // 3. Must not have genuine conflicting candidates
    if (!field.conflictingCandidates.empty()) {
        return false;
    }

    // 4. Must not have malformed OCR noise or token duplication loops
    if (hasMalformedOcrCharacters(field.final.value) || hasDuplicatedOcrTokens(field.final.value)) {
        return false;
    }

    // 5. Structural credit resolution OR OCR confidence baseline
    if (field.field == "credits" || field.field == "quality_points") {
        float structConf = 0.0f;
        if (isStrongStructuralCreditsEvidence(field, doc, allFields, structConf)) {
            return true;
        }
    }

    // OCR confidence baseline: must be above the configured minimum threshold (mediumThreshold)
    // Genuine low OCR confidence (< mediumThreshold) indicates corrupted/illegible OCR that legitimately requires AI
    float minTableConf = std::min(config_.mediumThreshold, 0.85f);
    float ocrConf = field.original.confidence > 0.0f ? field.original.confidence : field.final.confidence;
    if (ocrConf < minTableConf) {
        return false;
    }

    // 6. Check table membership from doc.tables or semantic relationship
    bool hasTableMembership = (field.semanticRelationship == "TABLE_COLUMN_ASSOCIATION");
    int page = field.original.source.page > 0 ? field.original.source.page : 1;
    for (const auto& tbl : doc.tables) {
        if (tbl.pageStart <= page && tbl.pageEnd >= page) {
            hasTableMembership = true;
            break;
        }
    }

    // 7. Check row companions (course code, credits, grade)
    bool hasValidRowCompanion = false;
    for (const auto& nb : field.nearbyEvidence) {
        if (nb.find("Code:") == 0 || nb.find("Credits:") == 0 || nb.find("Grade:") == 0 || nb.find("Term:") == 0) {
            hasValidRowCompanion = true;
            break;
        }
    }

    if (!hasValidRowCompanion) {
        std::string suffix = extractIdSuffix(field.fieldId);
        for (const auto& other : allFields) {
            if (&other == &field) continue;
            if (other.category == "course_record" && extractIdSuffix(other.fieldId) == suffix) {
                if (!other.final.value.empty() && (other.field == "course_code" || other.field == "credits" || other.field == "grade")) {
                    hasValidRowCompanion = true;
                    break;
                }
            }
        }
    }

    // 8. Field-specific lexical and pattern checks
    if (field.field == "course_title") {
        std::string trimmedVal = trim(field.final.value);
        if (trimmedVal.size() < 2) return false;

        int alphaCount = 0;
        for (char c : trimmedVal) {
            if (std::isalpha(static_cast<unsigned char>(c))) alphaCount++;
        }
        if (alphaCount < 2) return false;

        // Must not be a pure course code
        static const std::regex codePattern(R"(^[A-Z]{2,5}[-\s]?[0-9]{2,4}[A-Z]?$)", std::regex::icase);
        if (std::regex_match(trimmedVal, codePattern)) return false;

        return (hasTableMembership || hasValidRowCompanion);
    } else if (field.field == "course_code") {
        static const std::regex codePattern(R"(^(?:[A-Z]{2,5}[-\s]?[0-9]{2,4}[A-Z]?|[0-9]{2}[A-Z]{2,4}[0-9]{2,4}[A-Z]?)$)", std::regex::icase);
        return std::regex_match(trim(field.final.value), codePattern) && (hasTableMembership || hasValidRowCompanion);
    } else if (field.field == "credits" || field.field == "quality_points") {
        return isNumericString(field.final.value) && (hasTableMembership || hasValidRowCompanion);
    } else if (field.field == "grade") {
        return !field.final.value.empty() && (hasTableMembership || hasValidRowCompanion);
    }

    return (hasTableMembership && hasValidRowCompanion);
}

FieldResolutionAnalysis ResolutionAnalyzer::analyzeField(
    const EvidenceField& field,
    const EvidenceDocument& doc,
    const std::vector<EvidenceField>& allFields
) const noexcept {
    FieldResolutionAnalysis analysis;

    // 1. Structural, separator, placeholder, noise elements
    if (field.structuralClass == StructuralClass::Placeholder) {
        analysis.status = ResolutionStatus::AiUnresolved;
        analysis.strategy = ResolutionStrategy::None;
        analysis.isTrusted = true;
        analysis.llmRequired = false;
        analysis.reasonCode = "PLACEHOLDER";
        analysis.reasonMessage = "Placeholder content; excluded from resolution";
        return analysis;
    }

    if (field.structuralClass == StructuralClass::Noise) {
        analysis.status = ResolutionStatus::AiUnresolved;
        analysis.strategy = ResolutionStrategy::None;
        analysis.isTrusted = true;
        analysis.llmRequired = false;
        analysis.reasonCode = "NOISE";
        analysis.reasonMessage = "Noise artifact; excluded from resolution";
        return analysis;
    }

    if (isStructuralOrNonSemantic(field.structuralClass)) {
        analysis.status = ResolutionStatus::Deterministic;
        analysis.strategy = ResolutionStrategy::None;
        analysis.isTrusted = true;
        analysis.llmRequired = false;
        analysis.reasonCode = "STRUCTURAL_CONTENT";
        analysis.reasonMessage = "Structural content; preserved without resolution";
        return analysis;
    }

    // 2. Semantic validity check
    if (!field.isSemanticValid) {
        analysis.status = ResolutionStatus::AiUnresolved;
        analysis.strategy = ResolutionStrategy::None;
        analysis.isTrusted = false;
        analysis.llmRequired = false;
        analysis.reasonCode = "INVALID_FIELD";
        analysis.reasonMessage = "Invalid semantic field; excluded from resolution";
        return analysis;
    }

    // 3. Check for empty / insufficient evidence
    if (field.original.value.empty() && field.final.value.empty()) {
        analysis.status = ResolutionStatus::AiUnresolved;
        analysis.strategy = ResolutionStrategy::None;
        analysis.isTrusted = false;
        analysis.llmRequired = false;
        analysis.reasonCode = "INSUFFICIENT_EVIDENCE";
        analysis.reasonMessage = "Empty field value; insufficient evidence for resolution";
        analysis.problem = {
            "INSUFFICIENT_EVIDENCE",
            "Field " + field.field + " is empty with no extracted evidence.",
            "No source text or label could be extracted for this field.",
            "Checked document sections and candidate occurrences.",
            "No evidence exists in the document."
        };
        return analysis;
    }

    // 4. Check for supported LLM field types
    if (!field.field.empty() && !isSupportedLlmField(field.field)) {
        analysis.status = ResolutionStatus::Deterministic;
        analysis.strategy = ResolutionStrategy::None;
        analysis.isTrusted = true;
        analysis.llmRequired = false;
        analysis.reasonCode = "UNSUPPORTED_FIELD_TYPE";
        analysis.reasonMessage = "Field type not eligible for LLM resolution";
        return analysis;
    }

    // 5. Semantic Problem 1: Conflicting Candidates
    if (hasConflictingEvidence(field, allFields)) {
        analysis.status = ResolutionStatus::LlmRequired;
        analysis.strategy = ResolutionStrategy::TextLlm;
        analysis.isTrusted = false;
        analysis.llmRequired = true;
        analysis.isAmbiguous = true;
        analysis.reasonCode = "CONFLICTING_EVIDENCE";
        analysis.reasonMessage = "Multiple contradictory candidate values exist for " + field.field;
        analysis.problem = {
            "CONFLICTING_EVIDENCE",
            "Multiple conflicting candidate values were extracted for " + field.field + ".",
            "Multiple contradictory evidence blocks exist in the source document.",
            "Compared document sections, label associations, and spatial provenance.",
            "Conflicting candidate values require semantic arbitration."
        };
        return analysis;
    }

    // 6. Semantic Problem 2: Duplicated OCR Tokens / String Loops
    if (hasDuplicatedOcrTokens(field.final.value) || hasDuplicatedOcrTokens(field.original.value)) {
        analysis.status = ResolutionStatus::LlmRequired;
        analysis.strategy = ResolutionStrategy::TextLlm;
        analysis.isTrusted = false;
        analysis.llmRequired = true;
        analysis.isAmbiguous = true;
        analysis.reasonCode = "AMBIGUOUS_FIELD_VALUE";
        analysis.reasonMessage = "Extracted " + field.field + " contains duplicated text tokens";
        analysis.problem = {
            "AMBIGUOUS_FIELD_VALUE",
            "The extracted " + field.field + " contains duplicated content: '" + field.final.value + "'.",
            "The deterministic resolver cannot determine whether the repeated text is OCR duplication or the actual source value.",
            "Compared label/value association and nearby identity evidence.",
            "Multiple interpretations remain possible."
        };
        return analysis;
    }

    // 7. Semantic Problem 3: Malformed OCR Characters
    bool isMalformed = hasMalformedOcrCharacters(field.final.value) ||
                       (!field.correction.applied && hasMalformedOcrCharacters(field.original.value));

    if (isMalformed) {
        analysis.status = ResolutionStatus::LlmRequired;
        analysis.strategy = ResolutionStrategy::TextLlm;
        analysis.isTrusted = false;
        analysis.llmRequired = true;
        analysis.isAmbiguous = true;
        analysis.reasonCode = "MALFORMED_OCR";
        analysis.reasonMessage = "Extracted " + field.field + " contains malformed characters or OCR noise";
        analysis.problem = {
            "MALFORMED_OCR",
            "The extracted " + field.field + " is corrupted with OCR noise characters: '" + field.final.value + "'.",
            "Deterministic dictionary matching and confusion rules cannot safely repair the value.",
            "Applied confusion matrix and fuzzy dictionary candidate matching.",
            "Multiple valid dictionary repairs remain possible."
        };
        return analysis;
    }

    // 8. Semantic Problem 4: Table Column Ambiguity
    if (field.category == "course_record") {
        if (field.field == "credits" || field.field == "quality_points") {
            if (!isNumericString(field.final.value)) {
                analysis.status = ResolutionStatus::LlmRequired;
                analysis.strategy = ResolutionStrategy::TextLlm;
                analysis.isTrusted = false;
                analysis.llmRequired = true;
                analysis.isAmbiguous = true;
                analysis.reasonCode = "TABLE_COLUMN_AMBIGUITY";
                analysis.reasonMessage = "Credits cell '" + field.final.value + "' is not numeric; possible column misalignment";
                analysis.problem = {
                    "TABLE_COLUMN_AMBIGUITY",
                    "The credits value '" + field.final.value + "' is ambiguous and does not match the numeric credits schema.",
                    "Table column alignment or cell segmentation is ambiguous across adjacent table headers.",
                    "Validated numeric format against table header and cell column indices.",
                    "Cell value may belong to an adjacent grade, status, or title column."
                };
                return analysis;
            }
        } else if (field.field == "course_code") {
            static const std::regex validCodeRegex(R"(^[A-Z]{2,5}[-\s]?[0-9]{2,4}[A-Z]?$)", std::regex::icase);
            if (!std::regex_match(field.final.value, validCodeRegex) && field.final.value.size() > 12) {
                analysis.status = ResolutionStatus::LlmRequired;
                analysis.strategy = ResolutionStrategy::TextLlm;
                analysis.isTrusted = false;
                analysis.llmRequired = true;
                analysis.isAmbiguous = true;
                analysis.reasonCode = "TABLE_COLUMN_AMBIGUITY";
                analysis.reasonMessage = "Course code '" + field.final.value + "' appears merged with course description";
                analysis.problem = {
                    "TABLE_COLUMN_AMBIGUITY",
                    "Course code '" + field.final.value + "' exceeds typical code length and may contain merged title text.",
                    "Table cell boundary between course code and title is ambiguous.",
                    "Evaluated course code regex pattern against cell text.",
                    "Boundary between code and title requires semantic segmentation."
                };
                return analysis;
            }
        }
    }

    // 9. Semantic Decision: Deterministic Resolution

    // Check if field was deterministically corrected with high confidence
    if (field.correction.applied && field.correction.confidence >= 0.85f) {
        analysis.status = ResolutionStatus::Deterministic;
        analysis.strategy = ResolutionStrategy::None;
        analysis.isTrusted = true;
        analysis.llmRequired = false;
        analysis.reasonCode = "DETERMINISTIC_CORRECTION";
        analysis.reasonMessage = "Deterministic domain correction applied with high confidence";
        return analysis;
    }

    // Check if field is a structured course credit field with strong structural evidence
    if (field.category == "course_record" && (field.field == "credits" || field.field == "quality_points")) {
        float structConf = 0.0f;
        if (isStrongStructuralCreditsEvidence(field, doc, allFields, structConf)) {
            analysis.status = ResolutionStatus::Deterministic;
            analysis.strategy = ResolutionStrategy::None;
            analysis.isTrusted = true;
            analysis.llmRequired = false;
            analysis.reasonCode = "DETERMINISTIC_CREDITS_RESOLUTION";
            analysis.reasonMessage = "Credits accepted deterministically via strong table, column, and row structural evidence";
            return analysis;
        }
    }

    // Check if field is a well-formed structured course record cell with strong table/row evidence
    if (isStrongTableRowEvidence(field, doc, allFields)) {
        analysis.status = ResolutionStatus::Deterministic;
        analysis.strategy = ResolutionStrategy::None;
        analysis.isTrusted = true;
        analysis.llmRequired = false;
        analysis.reasonCode = "DETERMINISTIC_TABLE_RECORD";
        analysis.reasonMessage = "Structured table cell with verified column association and valid row companions";
        return analysis;
    }

    // Check if field has strong label-value association in EDR (for key-value pairs / metadata)
    bool hasStrongLabelValue = (field.category != "course_record") &&
                               (!field.labelText.empty() ||
                                (!field.labelBlockId.empty() && field.semanticRelationship == "VALUE_FOR_LABEL") ||
                                field.structuralClass == StructuralClass::StudentMetadata ||
                                field.structuralClass == StructuralClass::InstitutionData);

    if (hasStrongLabelValue && !field.final.value.empty()) {
        analysis.status = ResolutionStatus::Deterministic;
        analysis.strategy = ResolutionStrategy::None;
        analysis.isTrusted = true;
        analysis.llmRequired = false;
        analysis.reasonCode = "DETERMINISTIC_LABEL_VALUE";
        analysis.reasonMessage = "Strong label/value association established in EDR";
        return analysis;
    }

    // Check if normalization cleanly formatted the value
    if (field.normalization.applied && !field.final.value.empty() && field.final.confidence >= config_.highThreshold) {
        analysis.status = ResolutionStatus::Normalized;
        analysis.strategy = ResolutionStrategy::None;
        analysis.isTrusted = true;
        analysis.llmRequired = false;
        analysis.reasonCode = "NORMALIZED";
        analysis.reasonMessage = "Value normalized deterministically to canonical schema format";
        return analysis;
    }

    // 10. Semantic Decision: OCR Confident
    if (field.final.confidence >= config_.highThreshold && !field.final.value.empty()) {
        analysis.status = ResolutionStatus::OcrConfident;
        analysis.strategy = ResolutionStrategy::None;
        analysis.isTrusted = true;
        analysis.llmRequired = false;
        analysis.reasonCode = "OCR_CONFIDENT";
        analysis.reasonMessage = "Clean OCR extraction without semantic ambiguity";
        return analysis;
    }

    // 11. Low confidence unassociated text -> LLM Required
    analysis.status = ResolutionStatus::LlmRequired;
    analysis.strategy = ResolutionStrategy::TextLlm;
    analysis.isTrusted = false;
    analysis.llmRequired = true;
    analysis.isAmbiguous = true;
    analysis.reasonCode = "LOW_CONFIDENCE_UNRESOLVED";
    analysis.reasonMessage = "Unresolved field with low confidence requiring AI resolution";
    analysis.problem = {
        "AMBIGUOUS_FIELD_VALUE",
        "Low confidence extracted value for " + field.field + ": '" + field.final.value + "'.",
        "Extraction confidence is low and lacks strong structural or label association.",
        "Attempted deterministic dictionary and pattern validation.",
        "Value requires contextual AI verification."
    };

    return analysis;
}

DocumentResolutionSummary ResolutionAnalyzer::analyzeDocument(
    std::vector<EvidenceField>& fields,
    const EvidenceDocument& doc
) const noexcept {
    DocumentResolutionSummary summary;
    summary.totalFields = static_cast<int>(fields.size());

    for (auto& field : fields) {
        auto analysis = analyzeField(field, doc, fields);

        field.resolutionStatus = analysis.status;
        field.resolutionStrategy = analysis.strategy;
        field.problem = analysis.problem;
        field.final.trusted = analysis.isTrusted;
        field.final.llmRequired = analysis.llmRequired;
        field.llmEligible = analysis.llmRequired;
        field.llmReason = analysis.reasonCode;

        if (analysis.status == ResolutionStatus::Deterministic) {
            summary.deterministicCount++;
            field.resolutionMethod = "DETERMINISTIC";
        } else if (analysis.status == ResolutionStatus::OcrConfident) {
            summary.ocrConfidentCount++;
            field.resolutionMethod = "OCR";
        } else if (analysis.status == ResolutionStatus::Normalized) {
            summary.normalizedCount++;
            field.resolutionMethod = "NORMALIZED";
        } else if (analysis.status == ResolutionStatus::LlmRequired) {
            summary.llmRequiredCount++;
            field.resolutionMethod = "LLM_REQUIRED";
            if (field.field == "student_name" || field.field == "institution_name" || field.field == "major") {
                summary.hasMandatoryUnresolved = true;
            }
        } else if (analysis.status == ResolutionStatus::Resolved) {
            summary.resolvedCount++;
        } else {
            summary.aiUnresolvedCount++;
            if (field.field == "student_name" || field.field == "institution_name") {
                summary.hasMandatoryUnresolved = true;
            }
        }
    }

    summary.fastPathEligible = (summary.llmRequiredCount == 0);
    return summary;
}

float ResolutionAnalyzer::calculateStructuralCreditsConfidence(
    const EvidenceField& field,
    const EvidenceDocument& doc,
    const std::vector<EvidenceField>& allFields,
    std::string* outReason
) const noexcept {
    // 1. Value check
    std::string val = trim(field.final.value.empty() ? field.original.value : field.final.value);
    if (val.empty()) {
        if (outReason) *outReason = "Empty credit value";
        return 0.0f;
    }

    // Must not have malformed OCR characters or token duplication loops
    if (hasMalformedOcrCharacters(val) || hasDuplicatedOcrTokens(val)) {
        if (outReason) *outReason = "Malformed OCR characters or duplicated tokens in credit value";
        return 0.0f;
    }

    // Must not have unresolved conflicting candidates within this field
    if (!field.conflictingCandidates.empty()) {
        if (outReason) *outReason = "Field has explicit conflicting candidates";
        return 0.0f;
    }

    // Signal A: Numeric credit pattern validation (e.g. 1, 2, 3, 4, 5, 6, 1.0, 2.0, 3.0, 4.0)
    static const std::regex validCreditsRegex(R"(^[0-9]+(?:\.[0-9]+)?$)");
    if (!std::regex_match(val, validCreditsRegex)) {
        if (outReason) *outReason = "Value is not numeric";
        return 0.0f;
    }

    double numVal = 0.0;
    try {
        numVal = std::stod(val);
    } catch (...) {
        if (outReason) *outReason = "Failed to parse numeric credits";
        return 0.0f;
    }

    // Typical collegiate credit range check: 0.0 to 16.0
    float patternScore = 0.0f;
    if (numVal >= 0.5 && numVal <= 12.0) {
        patternScore = 0.25f; // Standard course credit value
    } else if (numVal >= 0.0 && numVal <= 16.0) {
        patternScore = 0.15f; // Valid credit value
    } else {
        if (outReason) *outReason = "Impossible credit value (outside plausible range 0.0-16.0)";
        return 0.0f;
    }

    // Signal B: Table Membership
    float tableScore = 0.0f;
    bool hasTableMembership = false;
    if (field.semanticRelationship == "TABLE_COLUMN_ASSOCIATION") {
        tableScore += 0.20f;
        hasTableMembership = true;
    }
    if (!field.labelBlockId.empty() && field.labelBlockId.find("table_") == 0) {
        tableScore += 0.10f;
        hasTableMembership = true;
    }
    int page = field.original.source.page > 0 ? field.original.source.page : 1;
    for (const auto& tbl : doc.tables) {
        if (tbl.pageStart <= page && tbl.pageEnd >= page) {
            tableScore += 0.10f;
            hasTableMembership = true;
            break;
        }
    }
    tableScore = std::min(tableScore, 0.25f);

    if (!hasTableMembership && !doc.tables.empty()) {
        tableScore -= 0.25f; // Outside table bounds
    }

    // Signal C: Credits Column Association
    float columnScore = 0.0f;
    if (field.field == "credits" || field.field == "quality_points") {
        if (field.sourceBlockId == "block_src_credits" || field.original.source.field == "credits") {
            columnScore += 0.15f;
        }
        if (field.semanticRelationship == "TABLE_COLUMN_ASSOCIATION") {
            columnScore += 0.10f;
        }
        for (const auto& tbl : doc.tables) {
            for (const auto& col : tbl.columns) {
                std::string uh = toUpper(col.name);
                std::string uc = toUpper(col.canonicalName);
                if (uh.find("CREDIT") != std::string::npos || uh == "CR" || uh == "HRS" || uh == "UNITS" ||
                    uc.find("CREDIT") != std::string::npos || uc == "CR" || uc == "HRS") {
                    columnScore += 0.05f;
                    break;
                }
            }
        }
    } else {
        columnScore -= 0.40f; // Outside credits column
    }
    columnScore = std::min(columnScore, 0.20f);

    // Signal D & E: Row Identity, Companions, and Spatial Alignment
    float rowScore = 0.0f;
    float spatialScore = 0.0f;
    std::string fieldSuffix = extractIdSuffix(field.fieldId);
    const EvidenceField* companionCode = nullptr;
    const EvidenceField* companionTitle = nullptr;
    const EvidenceField* companionGrade = nullptr;

    for (const auto& other : allFields) {
        if (&other == &field) continue;
        if (other.category != "course_record") continue;

        bool samePhysicalRow = false;
        if (!fieldSuffix.empty() && extractIdSuffix(other.fieldId) == fieldSuffix) {
            samePhysicalRow = true;
        } else if (other.original.source.page == field.original.source.page &&
                   other.original.source.bbox.height > 0 && field.original.source.bbox.height > 0) {
            int deltaY = std::abs(other.original.source.bbox.y - field.original.source.bbox.y);
            int maxH = std::max(other.original.source.bbox.height, field.original.source.bbox.height);
            if (deltaY <= maxH * 1.5) {
                samePhysicalRow = true;
            }
        }

        if (samePhysicalRow) {
            if (other.field == "course_code" && !other.final.value.empty()) {
                companionCode = &other;
            } else if (other.field == "course_title" && !other.final.value.empty()) {
                companionTitle = &other;
            } else if (other.field == "grade" && !other.final.value.empty()) {
                companionGrade = &other;
            } else if (other.field == "credits" && other.fieldId != field.fieldId) {
                // Competing credit candidate in the same physical row!
                if (trim(other.final.value) != val) {
                    if (outReason) *outReason = "Conflicting credit candidate in the same physical row";
                    return 0.0f;
                }
            }
        }
    }

    if (companionCode) rowScore += 0.20f;
    if (companionTitle) rowScore += 0.15f;
    if (companionGrade) rowScore += 0.10f;

    for (const auto& nb : field.nearbyEvidence) {
        if (nb.find("Code:") == 0) rowScore += 0.10f;
        else if (nb.find("Credits:") == 0) {
            std::string nbCredits = trim(nb.substr(8));
            if (nbCredits == val) rowScore += 0.10f;
        } else if (nb.find("Grade:") == 0 || nb.find("Status:") == 0) {
            rowScore += 0.05f;
        }
    }
    rowScore = std::min(rowScore, 0.30f);

    if (!companionCode && !companionTitle && field.nearbyEvidence.empty()) {
        rowScore -= 0.20f; // Isolated number with no course companions
    }

    // Spatial alignment with row companions
    if (field.original.source.bbox.width > 0 && field.original.source.bbox.height > 0) {
        spatialScore += 0.05f;
        const EvidenceField* companion = companionCode ? companionCode : companionTitle;
        if (companion && companion->original.source.bbox.height > 0) {
            int deltaY = std::abs(field.original.source.bbox.y - companion->original.source.bbox.y);
            int maxH = std::max(field.original.source.bbox.height, companion->original.source.bbox.height);
            if (deltaY <= maxH * 1.5) {
                spatialScore += 0.10f; // Vertically aligned within the row
            } else if (deltaY > maxH * 3) {
                spatialScore -= 0.25f; // Misaligned with row
            }
            if (field.original.source.bbox.x >= companion->original.source.bbox.x) {
                spatialScore += 0.05f; // Normal column order: code/title -> credits
            }
        }
    }
    spatialScore = std::clamp(spatialScore, -0.25f, 0.15f);

    // Signal F: OCR confidence baseline
    float ocrConf = field.original.confidence > 0.0f ? field.original.confidence : field.final.confidence;
    float ocrScore = 0.0f;
    if (ocrConf >= 0.85f) {
        ocrScore = 0.15f;
    } else if (ocrConf >= 0.70f) {
        ocrScore = 0.10f;
    } else if (ocrConf >= 0.50f) {
        ocrScore = 0.05f;
    } else if (ocrConf < 0.40f) {
        ocrScore = -0.20f;
    }

    float totalScore = patternScore + tableScore + columnScore + rowScore + spatialScore + ocrScore;
    totalScore = std::clamp(totalScore, 0.0f, 1.0f);

    if (outReason) {
        *outReason = "Structural score: " + std::to_string(totalScore);
    }
    return totalScore;
}

bool ResolutionAnalyzer::isStrongStructuralCreditsEvidence(
    const EvidenceField& field,
    const EvidenceDocument& doc,
    const std::vector<EvidenceField>& allFields,
    float& outConfidence
) const noexcept {
    if (field.field != "credits" && field.field != "quality_points") {
        return false;
    }

    std::string reason;
    float score = calculateStructuralCreditsConfidence(field, doc, allFields, &reason);
    outConfidence = score;

    // Check F: OCR confidence is valid OR structural evidence is sufficiently strong
    // Score must be >= 0.80f, and either OCR confidence is valid (>= 0.40f) or structural score is very high (>= 0.85f)
    float ocrConf = field.original.confidence > 0.0f ? field.original.confidence : field.final.confidence;
    if (score >= 0.80f && (ocrConf >= 0.40f || score >= 0.85f)) {
        return true;
    }

    return false;
}

} // namespace ocr::evidence

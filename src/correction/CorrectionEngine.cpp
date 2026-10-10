#include <edmo/evidence/correction/CorrectionEngine.hpp>

#include <algorithm>
#include <regex>
#include <sstream>

namespace edmo::evidence {

namespace {

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> splitWords(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream iss(text);
    std::string w;
    while (iss >> w) {
        words.push_back(w);
    }
    return words;
}

} // namespace

CorrectionEngine::CorrectionEngine(
    std::shared_ptr<CharacterConfusionMatrix> confusion,
    std::shared_ptr<DictionaryEngine> dictionary
) {
    confusion_ = confusion ? confusion : std::make_shared<CharacterConfusionMatrix>();
    dictionary_ = dictionary ? dictionary : std::make_shared<DictionaryEngine>();
}

CorrectionResult CorrectionEngine::correctText(
    const std::string& ocrText,
    const std::string& fieldType,
    float initialConfidence
) const {
    std::string trimmed = trim(ocrText);
    if (trimmed.empty()) {
        CorrectionResult res;
        res.value = "";
        res.confidence = initialConfidence;
        return res;
    }

    if (fieldType == "course_code") {
        return correctCourseCode(trimmed, initialConfidence);
    } else if (fieldType == "credits") {
        return correctCredits(trimmed, initialConfidence);
    } else if (fieldType == "grade") {
        return correctGrade(trimmed, initialConfidence);
    }

    return correctGeneralText(trimmed, initialConfidence);
}

CorrectionResult CorrectionEngine::correctCourseCode(
    const std::string& code,
    float initialConfidence
) const {
    CorrectionResult res;
    res.value = code;
    res.confidence = initialConfidence;
    res.method = CorrectionMethod::None;

    // Regex pattern matching standard course code: e.g. [A-Z]{2,5}\s*[0-9]{2,4}[A-Z]?
    static const std::regex courseCodePattern("^[A-Z]{2,5}\\s*[0-9]{2,4}[A-Z]?$", std::regex::icase);
    if (std::regex_match(code, courseCodePattern)) {
        // Already valid course code structure
        res.confidence = std::max(initialConfidence, 0.96f);
        return res;
    }

    // Check if code contains character substitutions in department prefix
    // (e.g. "ACCI 101" -> "ACCT 101", but safeguard "1CS101" from turning digits into letters in the numeric part)
    auto words = splitWords(code);
    if (words.size() == 2) {
        std::string dept = words[0];
        std::string num = words[1];

        // Ensure department part is checked against dictionary or character confusion
        auto deptCands = confusion_->generateCandidates(dept, 1, 5);
        for (const auto& c : deptCands) {
            std::string candidateCode = c.text + " " + num;
            if (std::regex_match(candidateCode, courseCodePattern)) {
                res.value = candidateCode;
                res.applied = true;
                res.method = CorrectionMethod::CharacterConfusion;
                res.confidence = 0.95f;
                res.provenance = "Course code prefix normalized";

                CorrectionCandidate cand;
                cand.value = candidateCode;
                cand.method = CorrectionMethod::CharacterConfusion;
                cand.confidence = 0.95f;
                cand.reason = "Normalized department prefix pattern";
                res.candidates.push_back(cand);
                return res;
            }
        }
    }

    return res;
}

CorrectionResult CorrectionEngine::correctCredits(
    const std::string& creditsStr,
    float initialConfidence
) const {
    CorrectionResult res;
    res.value = creditsStr;
    res.confidence = initialConfidence;
    res.method = CorrectionMethod::None;

    // Check if valid numeric
    static const std::regex numericPattern("^[0-9]+(?:\\.[0-9]+)?$");
    if (std::regex_match(creditsStr, numericPattern)) {
        res.confidence = std::max(initialConfidence, 0.98f);
        return res;
    }

    // Common OCR misreads in credit column: 'O' or 'o' -> '0', 'l' or 'I' or '|' -> '1', 'S' -> '5'
    std::string candidate = creditsStr;
    bool modified = false;
    if (candidate == "O" || candidate == "o") { candidate = "0"; modified = true; }
    else if (candidate == "l" || candidate == "I" || candidate == "|") { candidate = "1"; modified = true; }
    else if (candidate == "S" || candidate == "s") { candidate = "5"; modified = true; }
    else if (candidate == "B") { candidate = "8"; modified = true; }

    if (modified && std::regex_match(candidate, numericPattern)) {
        res.value = candidate;
        res.applied = true;
        res.method = CorrectionMethod::CharacterConfusion;
        res.confidence = 0.96f;
        res.provenance = "Corrected OCR digit misread in credits column";

        CorrectionCandidate c;
        c.value = candidate;
        c.method = CorrectionMethod::CharacterConfusion;
        c.confidence = 0.96f;
        c.reason = "OCR digit substitution in numeric credit column";
        res.candidates.push_back(c);
    }

    return res;
}

CorrectionResult CorrectionEngine::correctGrade(
    const std::string& gradeStr,
    float initialConfidence
) const {
    CorrectionResult res;
    res.value = gradeStr;
    res.confidence = initialConfidence;
    res.method = CorrectionMethod::None;

    if (dictionary_->isValidGrade(gradeStr)) {
        res.confidence = std::max(initialConfidence, 0.98f);
        return res;
    }

    // Grade substitutions: '0' -> 'O', '8' -> 'B', '1' -> 'I'
    auto cands = confusion_->generateCandidates(gradeStr, 1, 5);
    for (const auto& c : cands) {
        if (dictionary_->isValidGrade(c.text)) {
            res.value = c.text;
            res.applied = true;
            res.method = CorrectionMethod::CharacterConfusionAndDictionary;
            res.confidence = 0.96f;
            res.provenance = "Corrected grade against valid grading scale";

            CorrectionCandidate cand;
            cand.value = c.text;
            cand.method = CorrectionMethod::CharacterConfusionAndDictionary;
            cand.confidence = 0.96f;
            cand.reason = "Grading scale match";
            res.candidates.push_back(cand);
            return res;
        }
    }

    return res;
}

CorrectionResult CorrectionEngine::correctGeneralText(
    const std::string& text,
    float initialConfidence
) const {
    CorrectionResult res;
    res.value = text;
    res.confidence = initialConfidence;
    res.method = CorrectionMethod::None;

    auto words = splitWords(text);
    if (words.empty()) return res;

    // Fast path: If all words are already valid dictionary words, return immediately with boost
    bool allWordsValid = true;
    for (const auto& word : words) {
        if (!dictionary_->containsAny(word)) {
            allWordsValid = false;
            break;
        }
    }
    if (allWordsValid) {
        res.confidence = std::max(initialConfidence, 0.97f);
        return res;
    }

    std::vector<std::string> correctedWords;
    correctedWords.reserve(words.size());

    bool anyWordCorrected = false;
    float accumulatedConf = 0.0f;
    CorrectionMethod primaryMethod = CorrectionMethod::None;

    for (const auto& word : words) {
        // If word is already valid dictionary word (case-insensitive)
        if (dictionary_->containsAny(word)) {
            correctedWords.push_back(word);
            accumulatedConf += 0.98f;
            continue;
        }

        // 1. Try Character Confusion on this word
        auto cands = confusion_->generateCandidates(word, 2, 10);
        bool foundConfusionMatch = false;
        for (const auto& c : cands) {
            if (dictionary_->containsAny(c.text)) {
                correctedWords.push_back(c.text);
                anyWordCorrected = true;
                primaryMethod = CorrectionMethod::CharacterConfusionAndDictionary;
                accumulatedConf += (0.95f * c.score);
                foundConfusionMatch = true;
                break;
            }
        }

        if (foundConfusionMatch) continue;

        // 2. Try Fuzzy Match against domain terms if word length >= 3
        if (FuzzyMatcher::isEligibleForFuzzyMatch(word)) {
            auto matches = FuzzyMatcher::findMatches(word, dictionary_->domainTerms(), 0.82f, 3);
            if (!matches.empty() && matches[0].similarity >= 0.85f) {
                // If the match is a single word or exact length match
                if (matches[0].candidate.find(' ') == std::string::npos) {
                    correctedWords.push_back(matches[0].candidate);
                    anyWordCorrected = true;
                    primaryMethod = CorrectionMethod::FuzzyDictionaryMatch;
                    accumulatedConf += (0.90f * matches[0].similarity);
                    continue;
                }
            }
        }

        // Keep original word if no confident correction found
        correctedWords.push_back(word);
        accumulatedConf += initialConfidence;
    }

    // Check full phrase fuzzy match (e.g. "MATHEMAT1CS FOR C0MPUTING" -> "MATHEMATICS FOR COMPUTING")
    std::string reconstructed;
    for (size_t i = 0; i < correctedWords.size(); ++i) {
        if (i > 0) reconstructed += " ";
        reconstructed += correctedWords[i];
    }

    if (anyWordCorrected) {
        res.value = reconstructed;
        res.applied = true;
        res.method = primaryMethod;
        res.confidence = std::clamp(accumulatedConf / words.size(), 0.0f, 0.99f);
        res.provenance = "Deterministic vocabulary & confusion correction";

        CorrectionCandidate c;
        c.value = reconstructed;
        c.method = primaryMethod;
        c.confidence = res.confidence;
        c.similarityScore = FuzzyMatcher::normalizedLevenshtein(text, reconstructed);
        c.dictionaryScore = 1.0f;
        c.reason = "Character confusion and domain dictionary match";
        res.candidates.push_back(c);
    } else {
        // Try full phrase fuzzy search across domain vocabulary
        auto phraseMatches = FuzzyMatcher::findMatches(text, dictionary_->domainTerms(), 0.88f, 3);
        if (!phraseMatches.empty() && phraseMatches[0].similarity >= 0.88f) {
            res.value = phraseMatches[0].candidate;
            res.applied = true;
            res.method = CorrectionMethod::DomainVocabularyMatch;
            res.confidence = 0.95f * phraseMatches[0].similarity;
            res.provenance = "Full phrase domain vocabulary match";

            CorrectionCandidate c;
            c.value = phraseMatches[0].candidate;
            c.method = CorrectionMethod::DomainVocabularyMatch;
            c.confidence = res.confidence;
            c.similarityScore = phraseMatches[0].similarity;
            c.dictionaryScore = 1.0f;
            c.reason = "Fuzzy domain vocabulary phrase match";
            res.candidates.push_back(c);
        }
    }

    return res;
}

} // namespace edmo::evidence

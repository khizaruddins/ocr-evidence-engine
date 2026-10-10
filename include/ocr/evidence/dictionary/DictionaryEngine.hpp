#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace ocr::evidence {

enum class DictionaryType {
    General,
    Academic,
    Transcript,
    Grades,
    Degrees
};

/**
 * @brief Fast dictionary lookup engine with multi-vocabulary domain support and case-insensitivity.
 */
class DictionaryEngine {
public:
    DictionaryEngine();

    /**
     * @brief Load all dictionaries from directory path containing json dictionaries.
     */
    bool loadDictionariesFromDir(const std::filesystem::path& dirPath);

    /**
     * @brief Load dictionary from JSON file path.
     */
    bool loadDictionaryFile(const std::filesystem::path& filePath, DictionaryType type);

    /**
     * @brief Add a word/phrase to specific vocabulary.
     */
    void addTerm(const std::string& term, DictionaryType type);

    /**
     * @brief Check if exact term exists in specified dictionary.
     */
    [[nodiscard]] bool contains(std::string_view term, DictionaryType type) const noexcept;

    /**
     * @brief Check if exact term exists in ANY loaded dictionary.
     */
    [[nodiscard]] bool containsAny(std::string_view term) const noexcept;

    /**
     * @brief Score a term's dictionary match quality [0.0, 1.0].
     * Returns 1.0 for exact academic/transcript match, 0.9 for general match, 0.0 for unknown.
     */
    [[nodiscard]] float scoreTerm(std::string_view term) const noexcept;

    /**
     * @brief Check if grade is valid according to grading scale dictionary.
     */
    [[nodiscard]] bool isValidGrade(std::string_view grade) const noexcept;

    /**
     * @brief Get reference to all academic and transcript domain terms for fuzzy matching.
     */
    [[nodiscard]] const std::vector<std::string>& domainTerms() const noexcept {
        return domainTermsList_;
    }

private:
    void loadEmbeddedDefaults();
    static std::string normalize(std::string_view str);

    std::unordered_set<std::string> generalWords_;
    std::unordered_set<std::string> academicWords_;
    std::unordered_set<std::string> transcriptWords_;
    std::unordered_set<std::string> validGrades_;
    std::unordered_set<std::string> degreesList_;

    std::vector<std::string> domainTermsList_;
};

} // namespace ocr::evidence

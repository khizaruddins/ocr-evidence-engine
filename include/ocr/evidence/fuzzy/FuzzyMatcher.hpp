#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace edmo::evidence {

struct FuzzyMatchResult {
    std::string candidate;
    float similarity = 0.0f; // [0.0, 1.0]
    int distance = 0;
};

/**
 * @brief High-performance fuzzy string matching using normalized Levenshtein and Jaro-Winkler with length safeguards.
 */
class FuzzyMatcher {
public:
    /**
     * @brief Compute standard Levenshtein edit distance between two strings.
     */
    [[nodiscard]] static int levenshteinDistance(
        std::string_view s1,
        std::string_view s2
    ) noexcept;

    /**
     * @brief Compute normalized Levenshtein similarity score in range [0.0, 1.0].
     */
    [[nodiscard]] static float normalizedLevenshtein(
        std::string_view s1,
        std::string_view s2
    ) noexcept;

    /**
     * @brief Compute Jaro-Winkler similarity score in range [0.0, 1.0].
     */
    [[nodiscard]] static float jaroWinkler(
        std::string_view s1,
        std::string_view s2
    ) noexcept;

    /**
     * @brief Find best matching candidate from vocabulary dictionary.
     * Enforces safeguards: minimum token length >= 3, similarity threshold >= minSimilarity.
     */
    [[nodiscard]] static std::vector<FuzzyMatchResult> findMatches(
        std::string_view query,
        const std::vector<std::string>& candidates,
        float minSimilarity = 0.80f,
        size_t maxMatches = 5
    );

    /**
     * @brief Safeguard check: should fuzzy matching be permitted for this token?
     * Prevents over-correction on short abbreviations like "IN", "CS", "3", "A".
     */
    [[nodiscard]] static bool isEligibleForFuzzyMatch(std::string_view token) noexcept {
        return token.size() >= 3;
    }
};

} // namespace edmo::evidence

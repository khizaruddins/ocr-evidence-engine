#include <ocr/evidence/fuzzy/FuzzyMatcher.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace ocr::evidence {

int FuzzyMatcher::levenshteinDistance(
    std::string_view s1,
    std::string_view s2
) noexcept {
    const size_t m = s1.size();
    const size_t n = s2.size();
    if (m == 0) return static_cast<int>(n);
    if (n == 0) return static_cast<int>(m);

    std::vector<int> prevRow(n + 1);
    std::vector<int> currRow(n + 1);

    for (size_t j = 0; j <= n; ++j) {
        prevRow[j] = static_cast<int>(j);
    }

    for (size_t i = 1; i <= m; ++i) {
        currRow[0] = static_cast<int>(i);
        char c1 = static_cast<char>(std::toupper(static_cast<unsigned char>(s1[i - 1])));

        for (size_t j = 1; j <= n; ++j) {
            char c2 = static_cast<char>(std::toupper(static_cast<unsigned char>(s2[j - 1])));
            int cost = (c1 == c2) ? 0 : 1;
            currRow[j] = std::min({
                prevRow[j] + 1,       // deletion
                currRow[j - 1] + 1,   // insertion
                prevRow[j - 1] + cost // substitution
            });
        }
        prevRow = currRow;
    }

    return prevRow[n];
}

float FuzzyMatcher::normalizedLevenshtein(
    std::string_view s1,
    std::string_view s2
) noexcept {
    size_t maxLen = std::max(s1.size(), s2.size());
    if (maxLen == 0) return 1.0f;
    int dist = levenshteinDistance(s1, s2);
    return std::clamp(1.0f - (static_cast<float>(dist) / static_cast<float>(maxLen)), 0.0f, 1.0f);
}

float FuzzyMatcher::jaroWinkler(
    std::string_view s1,
    std::string_view s2
) noexcept {
    if (s1 == s2) return 1.0f;
    const int len1 = static_cast<int>(s1.size());
    const int len2 = static_cast<int>(s2.size());
    if (len1 == 0 || len2 == 0) return 0.0f;

    const int matchDistance = std::max(len1, len2) / 2 - 1;
    std::vector<bool> s1Matches(len1, false);
    std::vector<bool> s2Matches(len2, false);

    int matches = 0;
    for (int i = 0; i < len1; ++i) {
        int start = std::max(0, i - matchDistance);
        int end = std::min(i + matchDistance + 1, len2);
        char c1 = static_cast<char>(std::toupper(static_cast<unsigned char>(s1[i])));

        for (int j = start; j < end; ++j) {
            if (!s2Matches[j]) {
                char c2 = static_cast<char>(std::toupper(static_cast<unsigned char>(s2[j])));
                if (c1 == c2) {
                    s1Matches[i] = true;
                    s2Matches[j] = true;
                    matches++;
                    break;
                }
            }
        }
    }

    if (matches == 0) return 0.0f;

    // Transpositions
    int k = 0;
    int transpositions = 0;
    for (int i = 0; i < len1; ++i) {
        if (s1Matches[i]) {
            while (!s2Matches[k]) k++;
            char c1 = static_cast<char>(std::toupper(static_cast<unsigned char>(s1[i])));
            char c2 = static_cast<char>(std::toupper(static_cast<unsigned char>(s2[k])));
            if (c1 != c2) transpositions++;
            k++;
        }
    }

    float m = static_cast<float>(matches);
    float jaro = ((m / len1) + (m / len2) + ((m - transpositions / 2.0f) / m)) / 3.0f;

    // Winkler prefix adjustment
    int prefix = 0;
    for (int i = 0; i < std::min({len1, len2, 4}); ++i) {
        char c1 = static_cast<char>(std::toupper(static_cast<unsigned char>(s1[i])));
        char c2 = static_cast<char>(std::toupper(static_cast<unsigned char>(s2[i])));
        if (c1 == c2) prefix++;
        else break;
    }

    return std::clamp(jaro + 0.1f * prefix * (1.0f - jaro), 0.0f, 1.0f);
}

std::vector<FuzzyMatchResult> FuzzyMatcher::findMatches(
    std::string_view query,
    const std::vector<std::string>& candidates,
    float minSimilarity,
    size_t maxMatches
) {
    std::vector<FuzzyMatchResult> results;
    if (!isEligibleForFuzzyMatch(query)) return results;

    const size_t qLen = query.size();

    for (const auto& cand : candidates) {
        if (cand.empty()) continue;
        const size_t cLen = cand.size();
        size_t maxLen = std::max(qLen, cLen);
        size_t minLen = std::min(qLen, cLen);
        if (static_cast<float>(maxLen - minLen) / static_cast<float>(maxLen) > (1.0f - minSimilarity)) {
            continue; // Length difference mathematically exceeds max allowed distance
        }

        float sim = normalizedLevenshtein(query, cand);
        if (sim >= minSimilarity) {
            results.push_back({cand, sim, levenshteinDistance(query, cand)});
        }
    }

    std::sort(results.begin(), results.end(), [](const FuzzyMatchResult& a, const FuzzyMatchResult& b) {
        return a.similarity > b.similarity;
    });

    if (results.size() > maxMatches) {
        results.resize(maxMatches);
    }

    return results;
}

} // namespace ocr::evidence

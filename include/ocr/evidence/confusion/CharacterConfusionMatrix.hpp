#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace edmo::evidence {

struct ConfusionEntry {
    std::string from;
    std::string to;
    float weight = 0.90f;
    bool symmetric = true;
};

struct ScoredCandidate {
    std::string text;
    float score = 1.0f;
    int substitutionsCount = 0;
};

/**
 * @brief Character confusion matrix modeling visual OCR optical substitution patterns.
 */
class CharacterConfusionMatrix {
public:
    CharacterConfusionMatrix();

    /**
     * @brief Load substitutions from JSON configuration file.
     */
    bool loadFromFile(const std::filesystem::path& path);

    /**
     * @brief Load substitutions from JSON object.
     */
    bool loadFromJson(const nlohmann::json& j);

    /**
     * @brief Register a confusion substitution.
     */
    void addConfusion(const std::string& from, const std::string& to, float weight, bool symmetric = true);

    /**
     * @brief Generate candidate variations of an OCR token with substitution penalties.
     * @param maxSubstitutions Maximum allowable character substitutions per token (default 3).
     * @param maxCandidates Maximum number of candidate permutations to return (default 20).
     */
    [[nodiscard]] std::vector<ScoredCandidate> generateCandidates(
        const std::string& token,
        int maxSubstitutions = 3,
        size_t maxCandidates = 20
    ) const;

    /**
     * @brief Check if two characters/substrings are known OCR confusions.
     */
    [[nodiscard]] float getConfusionWeight(const std::string& from, const std::string& to) const noexcept;

private:
    void loadDefaultConfusions();

    // Map: original pattern -> list of (replacement, weight)
    std::map<std::string, std::vector<std::pair<std::string, float>>> substitutions_;
};

} // namespace edmo::evidence

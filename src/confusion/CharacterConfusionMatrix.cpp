#include <ocr/evidence/confusion/CharacterConfusionMatrix.hpp>

#include <algorithm>
#include <fstream>
#include <queue>
#include <set>

namespace ocr::evidence {

CharacterConfusionMatrix::CharacterConfusionMatrix() {
    loadDefaultConfusions();
}

void CharacterConfusionMatrix::loadDefaultConfusions() {
    // Digits <-> Letters
    addConfusion("0", "O", 0.95f, true);
    addConfusion("0", "o", 0.90f, true);
    addConfusion("0", "Q", 0.80f, true);
    addConfusion("1", "I", 0.98f, true);
    addConfusion("1", "l", 0.98f, true);
    addConfusion("1", "i", 0.90f, true);
    addConfusion("1", "|", 0.95f, true);
    addConfusion("2", "Z", 0.90f, true);
    addConfusion("2", "z", 0.85f, true);
    addConfusion("3", "E", 0.80f, true);
    addConfusion("4", "A", 0.80f, true);
    addConfusion("5", "S", 0.95f, true);
    addConfusion("5", "s", 0.90f, true);
    addConfusion("6", "b", 0.85f, true);
    addConfusion("6", "G", 0.80f, true);
    addConfusion("7", "T", 0.80f, true);
    addConfusion("8", "B", 0.95f, true);
    addConfusion("8", "&", 0.80f, true);
    addConfusion("9", "g", 0.85f, true);
    addConfusion("9", "q", 0.85f, true);

    // Multi-character ligatures
    addConfusion("rn", "m", 0.95f, true);
    addConfusion("cl", "d", 0.95f, true);
    addConfusion("vv", "w", 0.95f, true);
    addConfusion("li", "h", 0.85f, true);
    addConfusion("ci", "a", 0.85f, true);
    addConfusion("ri", "n", 0.80f, true);
    addConfusion("tl", "d", 0.75f, true);
}

void CharacterConfusionMatrix::addConfusion(
    const std::string& from,
    const std::string& to,
    float weight,
    bool symmetric
) {
    substitutions_[from].push_back({to, weight});
    if (symmetric && from != to) {
        substitutions_[to].push_back({from, weight});
    }
}

bool CharacterConfusionMatrix::loadFromFile(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) return false;
    try {
        std::ifstream in(path);
        nlohmann::json j;
        in >> j;
        return loadFromJson(j);
    } catch (...) {
        return false;
    }
}

bool CharacterConfusionMatrix::loadFromJson(const nlohmann::json& j) {
    if (!j.is_object() || !j.contains("confusions") || !j["confusions"].is_array()) {
        return false;
    }

    for (const auto& item : j["confusions"]) {
        if (!item.is_object()) continue;
        std::string from = item.value("from", "");
        std::string to = item.value("to", "");
        float weight = item.value("weight", 0.90f);
        bool symmetric = item.value("symmetric", true);
        if (!from.empty() && !to.empty()) {
            addConfusion(from, to, weight, symmetric);
        }
    }
    return true;
}

float CharacterConfusionMatrix::getConfusionWeight(
    const std::string& from,
    const std::string& to
) const noexcept {
    if (from == to) return 1.0f;
    auto it = substitutions_.find(from);
    if (it != substitutions_.end()) {
        for (const auto& [target, w] : it->second) {
            if (target == to) return w;
        }
    }
    return 0.0f;
}

std::vector<ScoredCandidate> CharacterConfusionMatrix::generateCandidates(
    const std::string& token,
    int maxSubstitutions,
    size_t maxCandidates
) const {
    std::vector<ScoredCandidate> results;
    if (token.empty()) return results;

    // BFS Queue: { current_string, current_score, substitutions_done, position_offset }
    struct State {
        std::string text;
        float score;
        int subs;
        size_t pos;
    };

    std::queue<State> q;
    std::set<std::string> visited;

    q.push({token, 1.0f, 0, 0});
    visited.insert(token);

    while (!q.empty() && results.size() < maxCandidates) {
        auto curr = q.front();
        q.pop();

        if (curr.text != token) {
            results.push_back({curr.text, curr.score, curr.subs});
        }

        if (curr.subs >= maxSubstitutions) {
            continue;
        }

        // Try substitutions from curr.pos onwards
        for (size_t i = curr.pos; i < curr.text.size(); ++i) {
            // 1. Single character substitution
            std::string ch(1, curr.text[i]);
            auto it = substitutions_.find(ch);
            if (it != substitutions_.end()) {
                for (const auto& [rep, weight] : it->second) {
                    std::string nextText = curr.text.substr(0, i) + rep + curr.text.substr(i + 1);
                    if (visited.find(nextText) == visited.end()) {
                        visited.insert(nextText);
                        q.push({nextText, curr.score * weight, curr.subs + 1, i + rep.size()});
                    }
                }
            }

            // 2. Two-character ligature substitution (e.g. rn -> m, cl -> d)
            if (i + 1 < curr.text.size()) {
                std::string twoCh = curr.text.substr(i, 2);
                auto it2 = substitutions_.find(twoCh);
                if (it2 != substitutions_.end()) {
                    for (const auto& [rep, weight] : it2->second) {
                        std::string nextText = curr.text.substr(0, i) + rep + curr.text.substr(i + 2);
                        if (visited.find(nextText) == visited.end()) {
                            visited.insert(nextText);
                            q.push({nextText, curr.score * weight, curr.subs + 1, i + rep.size()});
                        }
                    }
                }
            }
        }
    }

    // Sort results by score descending
    std::sort(results.begin(), results.end(), [](const ScoredCandidate& a, const ScoredCandidate& b) {
        return a.score > b.score;
    });

    if (results.size() > maxCandidates) {
        results.resize(maxCandidates);
    }

    return results;
}

} // namespace ocr::evidence

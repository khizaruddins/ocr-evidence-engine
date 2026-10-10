#include <ocr/evidence/structure/PlaceholderDetector.hpp>

#include <algorithm>
#include <cctype>

namespace ocr::evidence {

namespace {

std::string_view trimWhitespace(std::string_view sv) noexcept {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }
    return sv;
}

std::string toUpper(std::string_view sv) {
    std::string s(sv);
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return s;
}

} // namespace

bool PlaceholderDetector::isSeparatorLine(std::string_view text) noexcept {
    auto trimmed = trimWhitespace(text);
    if (trimmed.empty()) return true;

    // A separator line consists of repeated dashes, underscores, equals, dots, asterisks, or tildes
    size_t sepCharCount = 0;
    for (char c : trimmed) {
        if (c == '-' || c == '_' || c == '=' || c == '.' || c == '*' || c == '~' || std::isspace(static_cast<unsigned char>(c))) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                sepCharCount++;
            }
        } else {
            return false;
        }
    }

    return sepCharCount >= 2;
}

bool PlaceholderDetector::isPlaceholder(std::string_view text) noexcept {
    auto trimmed = trimWhitespace(text);
    if (trimmed.empty()) return true;

    if (isSeparatorLine(trimmed)) return true;

    std::string upper = toUpper(trimmed);

    if (upper == "N/A" || upper == "NA" || upper == "N.A." || upper == "N/D" ||
        upper == "NULL" || upper == "NIL" || upper == "NONE" ||
        upper == "UNKNOWN" || upper == "UNSPECIFIED" ||
        upper == "-" || upper == "--" || upper == "---" ||
        upper == "..." || upper == "...." || upper == "____" ||
        upper == "NOT APPLICABLE" || upper == "NO GRADE" ||
        upper == "TBD" || upper == "BLANK") {
        return true;
    }

    return false;
}

bool PlaceholderDetector::isLabeledPlaceholder(std::string_view text) noexcept {
    auto trimmed = trimWhitespace(text);
    auto colonPos = trimmed.find(':');
    if (colonPos != std::string_view::npos) {
        auto val = trimWhitespace(trimmed.substr(colonPos + 1));
        return isPlaceholder(val);
    }
    return isPlaceholder(trimmed);
}

} // namespace ocr::evidence

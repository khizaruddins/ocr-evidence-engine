#pragma once

#include <string_view>
#include <string>

namespace ocr::evidence {

/**
 * @brief Utility to detect placeholder, null, separator, and empty text values.
 */
class PlaceholderDetector {
public:
    /**
     * @brief Checks if text represents a placeholder or non-usable value.
     */
    [[nodiscard]] static bool isPlaceholder(std::string_view text) noexcept;

    /**
     * @brief Checks if text consists solely of separator characters (e.g., '---', '___', '===', '...').
     */
    [[nodiscard]] static bool isSeparatorLine(std::string_view text) noexcept;

    /**
     * @brief Strips leading label (e.g. "Code: ") and checks if the extracted value is a placeholder.
     */
    [[nodiscard]] static bool isLabeledPlaceholder(std::string_view text) noexcept;
};

} // namespace ocr::evidence

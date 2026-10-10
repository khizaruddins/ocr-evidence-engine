#pragma once

#include <cmath>

namespace edmo::evidence {

/**
 * @brief Precision-safe floating-point confidence comparator with configurable epsilon.
 */
class ConfidenceComparator {
public:
    static constexpr float kDefaultEpsilon = 0.001f;

    /**
     * @brief Determines if two confidence values are effectively equal within epsilon.
     */
    [[nodiscard]] static constexpr bool effectivelyEqual(
        float a,
        float b,
        float epsilon = kDefaultEpsilon
    ) noexcept {
        return std::abs(a - b) <= epsilon;
    }

    /**
     * @brief Determines if confidence 'a' meets or exceeds 'threshold' considering epsilon margin.
     * Evaluates as: a >= (threshold - epsilon)
     */
    [[nodiscard]] static constexpr bool atOrAbove(
        float a,
        float threshold,
        float epsilon = kDefaultEpsilon
    ) noexcept {
        return a >= (threshold - epsilon);
    }

    /**
     * @brief Determines if confidence 'a' is genuinely below 'threshold' beyond epsilon margin.
     * Evaluates as: a < (threshold - epsilon)
     */
    [[nodiscard]] static constexpr bool below(
        float a,
        float threshold,
        float epsilon = kDefaultEpsilon
    ) noexcept {
        return a < (threshold - epsilon);
    }
};

} // namespace edmo::evidence

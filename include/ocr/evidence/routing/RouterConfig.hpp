#pragma once

namespace ocr::evidence {

/**
 * @brief Routing thresholds and budget for determining whether to TRUST deterministic resolution or require LLM.
 */
struct RouterConfig {
    float highThreshold = 0.90f;   ///< Threshold above which evidence is trusted without LLM (default 0.90)
    float mediumThreshold = 0.85f; ///< Secondary threshold for aggressive correction evaluation
    float epsilon = 0.001f;        ///< Precision comparison epsilon margin (default 0.001)
    bool enableLlmRouting = true;  ///< When true, ambiguous fields generate LLM resolution tasks
    int maxLlmTasks = 50;          ///< Maximum budget for LLM tasks (default 50)
    bool enableDeduplication = true; ///< Prevent duplicate tasks for the same semantic field
};

} // namespace ocr::evidence

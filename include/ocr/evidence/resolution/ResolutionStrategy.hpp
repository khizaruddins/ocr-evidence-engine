#pragma once

#include <string_view>
#include <string>

namespace ocr::evidence {

/**
 * @brief Resolution strategy to be applied to an unresolved field.
 */
enum class ResolutionStrategy {
    None,               ///< No AI required; field is deterministic, OCR confident, or normalized.
    TextLlm,            ///< Targeted Text-based LLM resolution.
    TextLlmExpanded,    ///< Escalated Text-based LLM resolution with expanded context.
    Vlm,                ///< Vision-Language Model resolution (future-extensible).
    Multimodal          ///< Multimodal resolution (future-extensible).
};

[[nodiscard]] constexpr std::string_view resolutionStrategyToString(ResolutionStrategy s) noexcept {
    switch (s) {
        case ResolutionStrategy::None: return "NONE";
        case ResolutionStrategy::TextLlm: return "TEXT_LLM";
        case ResolutionStrategy::TextLlmExpanded: return "TEXT_LLM_EXPANDED";
        case ResolutionStrategy::Vlm: return "VLM";
        case ResolutionStrategy::Multimodal: return "MULTIMODAL";
    }
    return "NONE";
}

[[nodiscard]] inline ResolutionStrategy resolutionStrategyFromString(std::string_view sv) noexcept {
    if (sv == "NONE") return ResolutionStrategy::None;
    if (sv == "TEXT_LLM") return ResolutionStrategy::TextLlm;
    if (sv == "TEXT_LLM_EXPANDED") return ResolutionStrategy::TextLlmExpanded;
    if (sv == "VLM") return ResolutionStrategy::Vlm;
    if (sv == "MULTIMODAL") return ResolutionStrategy::Multimodal;
    return ResolutionStrategy::None;
}

/**
 * @brief AI execution workflow lifecycle state.
 */
enum class AiWorkflowState {
    NotStarted,
    InProgress,
    Completed,
    RetryRequired,
    Failed
};

[[nodiscard]] constexpr std::string_view aiWorkflowStateToString(AiWorkflowState s) noexcept {
    switch (s) {
        case AiWorkflowState::NotStarted: return "NOT_STARTED";
        case AiWorkflowState::InProgress: return "IN_PROGRESS";
        case AiWorkflowState::Completed: return "COMPLETED";
        case AiWorkflowState::RetryRequired: return "RETRY_REQUIRED";
        case AiWorkflowState::Failed: return "FAILED";
    }
    return "NOT_STARTED";
}

[[nodiscard]] inline AiWorkflowState aiWorkflowStateFromString(std::string_view sv) noexcept {
    if (sv == "NOT_STARTED") return AiWorkflowState::NotStarted;
    if (sv == "IN_PROGRESS") return AiWorkflowState::InProgress;
    if (sv == "COMPLETED") return AiWorkflowState::Completed;
    if (sv == "RETRY_REQUIRED") return AiWorkflowState::RetryRequired;
    if (sv == "FAILED") return AiWorkflowState::Failed;
    return AiWorkflowState::NotStarted;
}

} // namespace ocr::evidence

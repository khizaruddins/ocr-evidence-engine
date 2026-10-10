#pragma once

#include <string>
#include <string_view>

namespace edmo::evidence {

/**
 * @brief Zero-loss disposition fates for every OCR and semantic evidence element.
 */
enum class EvidenceFate {
    Preserved,    ///< Unmodified original evidence retained with high confidence
    Corrected,    ///< Deterministically corrected with high confidence
    Structured,   ///< Incorporated into structured domain records (e.g. table rows)
    Compacted,    ///< Compacted representation preserving semantic meaning
    Deduplicated, ///< Redundant artifact safely deduplicated (e.g. repeated page headers)
    Metadata,     ///< Document-level engine/page metadata
    Noise,        ///< Isolated visual artifact or non-content noise
    Chunked,      ///< Partitioned into semantic context chunks
    LlmContext,   ///< Targeted for downstream LLM resolution due to remaining ambiguity
    Unclassified  ///< Unclassified or unexpected content safely preserved
};

[[nodiscard]] constexpr std::string_view fateToString(EvidenceFate fate) noexcept {
    switch (fate) {
        case EvidenceFate::Preserved:    return "PRESERVED";
        case EvidenceFate::Corrected:    return "CORRECTED";
        case EvidenceFate::Structured:   return "STRUCTURED";
        case EvidenceFate::Compacted:    return "COMPACTED";
        case EvidenceFate::Deduplicated: return "DEDUPLICATED";
        case EvidenceFate::Metadata:     return "METADATA";
        case EvidenceFate::Noise:        return "NOISE";
        case EvidenceFate::Chunked:      return "CHUNKED";
        case EvidenceFate::LlmContext:   return "LLM_CONTEXT";
        case EvidenceFate::Unclassified: return "UNCLASSIFIED";
    }
    return "UNCLASSIFIED";
}

[[nodiscard]] inline EvidenceFate fateFromString(std::string_view sv) noexcept {
    if (sv == "PRESERVED")    return EvidenceFate::Preserved;
    if (sv == "CORRECTED")    return EvidenceFate::Corrected;
    if (sv == "STRUCTURED")   return EvidenceFate::Structured;
    if (sv == "COMPACTED")    return EvidenceFate::Compacted;
    if (sv == "DEDUPLICATED") return EvidenceFate::Deduplicated;
    if (sv == "METADATA")     return EvidenceFate::Metadata;
    if (sv == "NOISE")        return EvidenceFate::Noise;
    if (sv == "CHUNKED")      return EvidenceFate::Chunked;
    if (sv == "LLM_CONTEXT")  return EvidenceFate::LlmContext;
    return EvidenceFate::Unclassified;
}

/**
 * @brief Method by which a correction was generated and verified.
 */
enum class CorrectionMethod {
    None,
    ExactDictionaryMatch,
    CharacterConfusion,
    CharacterConfusionAndDictionary,
    FuzzyDictionaryMatch,
    DomainVocabularyMatch,
    ContextualCorrection,
    MultiHypothesisFusion,
    LlmResolution
};

[[nodiscard]] constexpr std::string_view correctionMethodToString(CorrectionMethod m) noexcept {
    switch (m) {
        case CorrectionMethod::None:                            return "NONE";
        case CorrectionMethod::ExactDictionaryMatch:           return "EXACT_DICTIONARY_MATCH";
        case CorrectionMethod::CharacterConfusion:             return "CHARACTER_CONFUSION";
        case CorrectionMethod::CharacterConfusionAndDictionary: return "CHARACTER_CONFUSION_AND_DICTIONARY";
        case CorrectionMethod::FuzzyDictionaryMatch:           return "FUZZY_DICTIONARY_MATCH";
        case CorrectionMethod::DomainVocabularyMatch:          return "DOMAIN_VOCABULARY_MATCH";
        case CorrectionMethod::ContextualCorrection:           return "CONTEXTUAL_CORRECTION";
        case CorrectionMethod::MultiHypothesisFusion:          return "MULTI_HYPOTHESIS_FUSION";
        case CorrectionMethod::LlmResolution:                  return "LLM_RESOLUTION";
    }
    return "NONE";
}

} // namespace edmo::evidence

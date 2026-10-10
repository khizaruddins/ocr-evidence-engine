cmake_minimum_required(VERSION 3.22)

project(ocr-evidence-engine
    VERSION 0.2.0
    DESCRIPTION "OCR Evidence Intelligence Engine v0.2 (C++20)"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# Dependencies
include(FetchContent)

# 1. nlohmann_json
find_package(nlohmann_json 3.11 QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "Fetching nlohmann_json via FetchContent...")
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# 2. GoogleTest for unit testing
option(BUILD_TESTING "Build the testing tree." ON)
if(BUILD_TESTING)
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        message(STATUS "Fetching GoogleTest via FetchContent...")
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
            GIT_SHALLOW TRUE
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
    enable_testing()
endif()

# Core Library Sources
set(OCR_EVIDENCE_SOURCES
    src/parser/EdrParser.cpp
    src/structure/PlaceholderDetector.cpp
    src/structure/StructuralClassifier.cpp
    src/structure/CourseRowDetector.cpp
    src/structure/FieldCandidateValidator.cpp
    src/extraction/TranscriptFieldExtractor.cpp
    src/normalization/FieldValueNormalizer.cpp
    src/confidence/ConfidenceEngine.cpp
    src/confusion/CharacterConfusionMatrix.cpp
    src/dictionary/DictionaryEngine.cpp
    src/fuzzy/FuzzyMatcher.cpp
    src/correction/CorrectionEngine.cpp
    src/correction/ContextualCorrectionEngine.cpp
    src/fusion/EvidenceFusionEngine.cpp
    src/resolution/ResolutionAnalyzer.cpp
    src/routing/ConfidenceRouter.cpp
    src/context/TargetedLlmContextBuilder.cpp
    src/validation/ZeroLossAuditor.cpp
    src/validation/DocumentIdentityValidator.cpp
    src/validation/FinalDocumentValidator.cpp
    src/patch/LlmPatchValidator.cpp
    src/patch/EdrPatchEngine.cpp
    src/projection/FinalDocumentProjector.cpp
    src/pipeline/EvidencePipeline.cpp
    src/serialization/EvidenceJsonSerializer.cpp
)

# Core Library Target
add_library(ocr_evidence_core STATIC ${OCR_EVIDENCE_SOURCES})
target_include_directories(ocr_evidence_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(ocr_evidence_core PUBLIC nlohmann_json::nlohmann_json)

# CLI Application Target
add_executable(ocr-evidence src/main.cpp)
target_link_libraries(ocr-evidence PRIVATE ocr_evidence_core)

# Testing Target
if(BUILD_TESTING)
    set(TEST_SOURCES
        tests/main.cpp
        tests/unit/structure_tests.cpp
        tests/unit/regression_routing_tests.cpp
        tests/unit/normalizer_tests.cpp
        tests/unit/comparator_tests.cpp
        tests/unit/patch_validator_tests.cpp
        tests/unit/patch_fusion_tests.cpp
        tests/unit/reverse_path_tests.cpp
        tests/unit/final_projector_tests.cpp
        tests/unit/final_validator_tests.cpp
        tests/unit/confidence_tests.cpp
        tests/unit/correction_tests.cpp
        tests/unit/dictionary_tests.cpp
        tests/unit/fuzzy_tests.cpp
        tests/unit/fusion_tests.cpp
        tests/unit/routing_tests.cpp
        tests/unit/resolution_analyzer_tests.cpp
        tests/unit/context_tests.cpp
        tests/unit/zero_loss_tests.cpp
        tests/integration/pipeline_tests.cpp
        tests/integration/golden_tests.cpp
    )

    add_executable(ocr_evidence_tests ${TEST_SOURCES})
    target_link_libraries(ocr_evidence_tests PRIVATE
        ocr_evidence_core
        GTest::gtest
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(ocr_evidence_tests)
endif()

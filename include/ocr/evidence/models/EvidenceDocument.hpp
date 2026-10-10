#pragma once

#include <ocr/evidence/models/EvidenceField.hpp>
#include <ocr/evidence/models/EvidencePage.hpp>
#include <ocr/evidence/models/EvidenceSection.hpp>
#include <ocr/evidence/models/EvidenceTable.hpp>

#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace ocr::evidence {

struct DocumentMetadata {
    std::string id;
    std::string type = "transcript";
    int pageCount = 1;
    std::string source;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"id", id},
            {"type", type},
            {"page_count", pageCount},
            {"source", source}
        };
    }
};

struct EngineInfo {
    std::string name = "ocr-engine";
    std::string provider = "tesseract";
    std::string version = "0.1.0";

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"name", name},
            {"provider", provider},
            {"version", version}
        };
    }
};

/**
 * @brief Top-level enriched evidence document containing all pages, tables, fields, and accounting.
 */
struct EvidenceDocument {
    std::string schemaVersion = "evidence-0.1";
    DocumentMetadata metadata;
    EngineInfo engine;

    std::vector<EvidencePage> pages;
    std::vector<EvidenceSection> sections;
    std::vector<EvidenceTable> tables;
    std::vector<EvidenceField> fields;

    std::map<std::string, std::string> identity;
    std::map<std::string, std::string> institution;
    std::map<std::string, std::string> academic;

    std::vector<std::string> unclassifiedContent;
    ConfidenceSummary confidence;

    // Preserved raw metadata or extension properties
    nlohmann::json rawExtensions = nlohmann::json::object();

    [[nodiscard]] nlohmann::json toFullEvidenceJson() const {
        nlohmann::json j{
            {"schema_version", schemaVersion},
            {"document", metadata.toJson()},
            {"engine", engine.toJson()},
            {"confidence", confidence.toJson()}
        };

        if (!identity.empty()) {
            j["identity"] = identity;
        }
        if (!institution.empty()) {
            j["institution"] = institution;
        }
        if (!academic.empty()) {
            j["academic"] = academic;
        }

        nlohmann::json pagesJson = nlohmann::json::array();
        for (const auto& p : pages) {
            pagesJson.push_back(p.toJson(true));
        }
        j["pages"] = std::move(pagesJson);

        nlohmann::json fieldsJson = nlohmann::json::array();
        for (const auto& f : fields) {
            fieldsJson.push_back(f.toJson());
        }
        j["fields"] = std::move(fieldsJson);

        nlohmann::json tablesJson = nlohmann::json::array();
        for (const auto& t : tables) {
            tablesJson.push_back(t.toJson(true));
        }
        j["tables"] = std::move(tablesJson);

        if (!unclassifiedContent.empty()) {
            j["unclassified_content"] = unclassifiedContent;
        }

        return j;
    }
};

} // namespace ocr::evidence

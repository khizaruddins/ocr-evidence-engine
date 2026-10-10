#include <edmo/evidence/parser/EdrParser.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

namespace edmo::evidence {

namespace {

inline std::string toUpper(std::string_view sv) {
    std::string res;
    res.reserve(sv.size());
    for (char c : sv) {
        res.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return res;
}

inline std::string_view trimWhitespace(std::string_view sv) noexcept {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }
    return sv;
}

inline std::string normalizeCanonicalColumn(std::string_view name) {
    std::string s = toUpper(trimWhitespace(name));
    if (s == "GRADE" || s == "GRD" || s == "GR" || s == "FINAL GRADE" || s == "FINAL_GRADE" || s == "MARK" || s == "MARKS" || s == "LETTER GRADE") {
        return "grade";
    }
    if (s == "COURSE CODE" || s == "COURSE_CODE" || s == "CODE" || s == "CRS" || s == "SUBJ" || s == "COURSE NO" || s == "COURSE_NO" || s == "SUBJECT") {
        return "course_code";
    }
    if (s == "COURSE TITLE" || s == "COURSE_TITLE" || s == "TITLE" || s == "DESCRIPTION" || s == "COURSE DESCRIPTION" || s == "COURSE_DESCRIPTION" || s == "SUBJECT TITLE") {
        return "course_title";
    }
    if (s == "CREDITS" || s == "CREDIT" || s == "CR" || s == "HRS" || s == "HOURS" || s == "UNIT" || s == "UNITS") {
        return "credits";
    }
    if (s == "QUALITY POINTS" || s == "QUALITY_POINTS" || s == "POINTS" || s == "PTS" || s == "QP") {
        return "quality_points";
    }
    if (s == "TERM" || s == "SEMESTER" || s == "SESSION") {
        return "term";
    }
    return std::string(name);
}

} // namespace

EvidenceDocument EdrParser::parseFile(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Full EDR file does not exist: " + path.string());
    }

    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("Cannot open Full EDR file: " + path.string());
    }

    nlohmann::json j;
    in >> j;
    return parseJson(j);
}

EvidenceDocument EdrParser::parseJson(const nlohmann::json& root) {
    EvidenceDocument doc;
    if (!root.is_object()) return doc;

    doc.schemaVersion = root.value("schema_version", "edr-0.1");

    // 1. Document metadata
    if (root.contains("document") && root["document"].is_object()) {
        const auto& docObj = root["document"];
        doc.metadata.id = docObj.value("id", "doc_unknown");
        doc.metadata.type = docObj.value("type", "transcript");
        doc.metadata.pageCount = docObj.value("page_count", 1);
        doc.metadata.source = docObj.value("source", "");
    }

    // 2. Engine info
    if (root.contains("engine") && root["engine"].is_object()) {
        const auto& engObj = root["engine"];
        doc.engine.name = engObj.value("name", "edmo-ocr");
        doc.engine.provider = engObj.value("provider", "tesseract");
        doc.engine.version = engObj.value("version", "0.1.0");
    }

    // 3. Overall confidence summary
    if (root.contains("confidence") && root["confidence"].is_object()) {
        doc.confidence = ConfidenceSummary::fromJson(root["confidence"]);
    }

    // 4. Normalized domain entity blocks
    if (root.contains("identity") && root["identity"].is_object()) {
        for (const auto& [k, v] : root["identity"].items()) {
            if (v.is_string()) doc.identity[k] = v.get<std::string>();
            else if (!v.is_null()) doc.identity[k] = v.dump();
        }
    }

    if (root.contains("institution") && root["institution"].is_object()) {
        for (const auto& [k, v] : root["institution"].items()) {
            if (v.is_string()) doc.institution[k] = v.get<std::string>();
            else if (!v.is_null()) doc.institution[k] = v.dump();
        }
    }

    if (root.contains("academic") && root["academic"].is_object()) {
        for (const auto& [k, v] : root["academic"].items()) {
            if (v.is_string()) doc.academic[k] = v.get<std::string>();
            else if (!v.is_null()) doc.academic[k] = v.dump();
        }
    }

    // 5. Pages & blocks
    if (root.contains("pages") && root["pages"].is_array()) {
        for (const auto& pageObj : root["pages"]) {
            if (!pageObj.is_object()) continue;

            EvidencePage page;
            page.pageNumber = pageObj.value("page_number", 1);
            page.width = pageObj.value("width", 0);
            page.height = pageObj.value("height", 0);
            page.rotation = pageObj.value("rotation", 0);

            if (pageObj.contains("confidence") && pageObj["confidence"].is_object()) {
                page.confidence = ConfidenceSummary::fromJson(pageObj["confidence"]);
            }

            if (pageObj.contains("blocks") && pageObj["blocks"].is_array()) {
                for (const auto& blockObj : pageObj["blocks"]) {
                    if (!blockObj.is_object()) continue;

                    EvidenceBlock block;
                    block.id = blockObj.value("id", "");
                    block.text = blockObj.value("text", "");
                    block.type = blockTypeFromString(blockObj.value("type", "unknown"));
                    block.label = blockObj.value("label", "");
                    block.value = blockObj.value("value", "");

                    if (blockObj.contains("confidence") && blockObj["confidence"].is_object()) {
                        block.confidence = ConfidenceSummary::fromJson(blockObj["confidence"]);
                    }

                    if (blockObj.contains("source") && blockObj["source"].is_object()) {
                        block.source = EvidenceSource::fromJson(blockObj["source"]);
                    } else {
                        block.source.page = page.pageNumber;
                    }

                    page.blocks.push_back(std::move(block));
                }
            }

            doc.pages.push_back(std::move(page));
        }
    }

    // 6. Sections
    if (root.contains("sections") && root["sections"].is_array()) {
        for (const auto& secObj : root["sections"]) {
            if (!secObj.is_object()) continue;

            EvidenceSection sec;
            sec.id = secObj.value("id", "");
            sec.heading = secObj.value("heading", "");
            sec.sectionType = secObj.value("section_type", "body");
            sec.pageStart = secObj.value("page_start", 1);
            sec.pageEnd = secObj.value("page_end", 1);

            if (secObj.contains("confidence") && secObj["confidence"].is_object()) {
                sec.confidence = ConfidenceSummary::fromJson(secObj["confidence"]);
            }
            if (secObj.contains("source") && secObj["source"].is_object()) {
                sec.source = EvidenceSource::fromJson(secObj["source"]);
            }

            if (secObj.contains("blocks") && secObj["blocks"].is_array()) {
                for (const auto& blockObj : secObj["blocks"]) {
                    if (!blockObj.is_object()) continue;
                    EvidenceBlock block;
                    block.id = blockObj.value("id", "");
                    block.text = blockObj.value("text", "");
                    block.type = blockTypeFromString(blockObj.value("type", "unknown"));
                    block.label = blockObj.value("label", "");
                    block.value = blockObj.value("value", "");

                    if (blockObj.contains("confidence") && blockObj["confidence"].is_object()) {
                        block.confidence = ConfidenceSummary::fromJson(blockObj["confidence"]);
                    }
                    if (blockObj.contains("source") && blockObj["source"].is_object()) {
                        block.source = EvidenceSource::fromJson(blockObj["source"]);
                    }
                    sec.blocks.push_back(std::move(block));
                }
            }

            doc.sections.push_back(std::move(sec));
        }
    }

    // 7. Tables
    if (root.contains("tables") && root["tables"].is_array()) {
        for (const auto& tblObj : root["tables"]) {
            if (!tblObj.is_object()) continue;

            EvidenceTable tbl;
            tbl.id = tblObj.value("id", "");
            tbl.tableType = tblObj.value("table_type", "generic_table");
            tbl.caption = tblObj.value("caption", "");
            tbl.pageStart = tblObj.value("page_start", 1);
            tbl.pageEnd = tblObj.value("page_end", 1);
            tbl.hasHeader = tblObj.value("has_header", false);

            if (tblObj.contains("confidence") && tblObj["confidence"].is_object()) {
                tbl.confidence = ConfidenceSummary::fromJson(tblObj["confidence"]);
            }
            if (tblObj.contains("source") && tblObj["source"].is_object()) {
                tbl.source = EvidenceSource::fromJson(tblObj["source"]);
            }

            if (tblObj.contains("columns") && tblObj["columns"].is_array()) {
                for (const auto& colObj : tblObj["columns"]) {
                    if (!colObj.is_object()) continue;
                    EvidenceColumn col;
                    col.name = colObj.value("name", "");
                    col.canonicalName = normalizeCanonicalColumn(colObj.value("canonical_name", col.name));
                    col.columnIndex = colObj.value("column_index", 0);
                    col.confidence = colObj.value("confidence", 1.0f);
                    tbl.columns.push_back(std::move(col));
                }
            }

            // Fallback for tables that define "headers" array instead of "columns"
            if (tbl.columns.empty() && tblObj.contains("headers") && tblObj["headers"].is_array()) {
                int colIdx = 0;
                for (const auto& hItem : tblObj["headers"]) {
                    EvidenceColumn col;
                    col.columnIndex = colIdx++;
                    col.confidence = 1.0f;
                    if (hItem.is_string()) {
                        col.name = hItem.get<std::string>();
                        col.canonicalName = normalizeCanonicalColumn(col.name);
                    } else if (hItem.is_object()) {
                        col.name = hItem.value("name", hItem.value("text", ""));
                        col.canonicalName = normalizeCanonicalColumn(hItem.value("canonical_name", col.name));
                        col.confidence = hItem.value("confidence", 1.0f);
                    }
                    tbl.columns.push_back(std::move(col));
                }
            }

            if (tblObj.contains("rows") && tblObj["rows"].is_array()) {
                int rIdx = 0;
                for (const auto& rowObj : tblObj["rows"]) {
                    if (!rowObj.is_object()) continue;
                    EvidenceRow row;
                    row.rowIndex = rIdx++;
                    row.isHeader = rowObj.value("is_header", false);
                    row.confidence = rowObj.value("confidence", 1.0f);

                    if (rowObj.contains("source") && rowObj["source"].is_object()) {
                        row.source = EvidenceSource::fromJson(rowObj["source"]);
                    } else {
                        row.source.tableId = tbl.id;
                        row.source.page = tbl.pageStart;
                    }

                    // Cells
                    if (rowObj.contains("cells") && rowObj["cells"].is_array()) {
                        int cIdx = 0;
                        for (const auto& cellObj : rowObj["cells"]) {
                            if (!cellObj.is_object()) continue;
                            EvidenceCell cell;
                            cell.text = cellObj.value("text", "");
                            cell.confidence = cellObj.value("confidence", 1.0f);
                            cell.colSpan = cellObj.value("colspan", 1);
                            cell.rowSpan = cellObj.value("rowspan", 1);
                            cell.colIndex = cIdx;
                            cell.rowIndex = row.rowIndex;

                            if (cellObj.contains("name") && cellObj["name"].is_string()) {
                                cell.columnName = cellObj["name"].get<std::string>();
                            } else if (cellObj.contains("column_name") && cellObj["column_name"].is_string()) {
                                cell.columnName = cellObj["column_name"].get<std::string>();
                            } else if (cIdx < static_cast<int>(tbl.columns.size())) {
                                cell.columnName = tbl.columns[cIdx].name;
                            }

                            if (cellObj.contains("canonical_name") && cellObj["canonical_name"].is_string()) {
                                cell.canonicalColumnName = normalizeCanonicalColumn(cellObj["canonical_name"].get<std::string>());
                            } else if (cIdx < static_cast<int>(tbl.columns.size())) {
                                cell.canonicalColumnName = tbl.columns[cIdx].canonicalName;
                            } else if (!cell.columnName.empty()) {
                                cell.canonicalColumnName = normalizeCanonicalColumn(cell.columnName);
                            }

                            if (cellObj.contains("fate") && cellObj["fate"].is_string()) {
                                cell.fate = fateFromString(cellObj["fate"].get<std::string>());
                            }

                            if (cellObj.contains("source") && cellObj["source"].is_object()) {
                                cell.source = EvidenceSource::fromJson(cellObj["source"]);
                            } else {
                                cell.source = row.source;
                                cell.source.cell = cIdx;
                            }

                            // If cell has canonical name and text, map to row.canonicalFields
                            if (!cell.canonicalColumnName.empty() && !cell.text.empty()) {
                                if (row.canonicalFields.find(cell.canonicalColumnName) == row.canonicalFields.end()) {
                                    row.canonicalFields[cell.canonicalColumnName] = cell.text;
                                }
                            }

                            row.cells.push_back(std::move(cell));
                            cIdx++;
                        }
                    }

                    // Direct canonical key-value mappings on row (e.g. course_code, course_title, credits, grade)
                    for (const auto& [k, v] : rowObj.items()) {
                        if (k != "cells" && k != "source" && k != "is_header" && k != "confidence") {
                            std::string normKey = normalizeCanonicalColumn(k);
                            if (v.is_string()) {
                                row.canonicalFields[normKey] = v.get<std::string>();
                            } else if (v.is_number_integer()) {
                                row.canonicalFields[normKey] = std::to_string(v.get<int64_t>());
                            } else if (v.is_number_float()) {
                                row.canonicalFields[normKey] = std::to_string(v.get<double>());
                            }
                        }
                    }

                    tbl.rows.push_back(std::move(row));
                }
            }

            doc.tables.push_back(std::move(tbl));
        }
    }

    // 8. Unclassified content
    if (root.contains("unclassified_content") && root["unclassified_content"].is_array()) {
        for (const auto& uc : root["unclassified_content"]) {
            if (uc.is_string()) {
                doc.unclassifiedContent.push_back(uc.get<std::string>());
            } else if (!uc.is_null()) {
                doc.unclassifiedContent.push_back(uc.dump());
            }
        }
    }

    return doc;
}

} // namespace edmo::evidence

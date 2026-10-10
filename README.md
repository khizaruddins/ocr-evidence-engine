# Evidence Intelligence Engine v0.1

High-Performance C++20 Semantic Evidence Processing Layer for Document Intelligence.

---

## Key Features

- **Zero-Loss Provenance**: Guarantees 100% preservation of original OCR coordinates, token distributions, and source structures (`unaccounted_loss = 0`).
- **Deterministic OCR Correction**: Resolves optical scanning degradations via character confusion matrices, domain dictionaries, and normalized fuzzy matching.
- **Multi-Level Confidence Engine**: Computes token, block, table, and multi-factor field confidence scores.
- **Evidence Fusion**: Fuses original OCR, correction candidates, dictionary validation, and multi-hypothesis OCR passes.
- **Selective LLM Routing**: Bypasses LLM inference for clean documents (`llm_required: false, tasks: []`), routing strictly unresolved ambiguities.
- **Ultra-Low Latency**: Performance target < 100ms per page in pure C++20.

---

## Build Instructions

```bash
cd ocr-evidence-engine
cmake -B build -S .
cmake --build build -j$(nproc)
```

## Running Unit & Integration Tests

```bash
ctest --test-dir build --output-on-failure
# Or directly:
./build/bin/edmo_evidence_tests
```

## Running the CLI

```bash
./build/bin/ocr-evidence \
  --input examples/input/sophia_jones_edr.json \
  --output-dir output/ \
  --benchmark \
  --verbose
```

## Output Artifacts

- `output/evidence-full.json`: Complete enriched evidence model.
- `output/evidence-llm.json`: Targeted minimal LLM context.
- `output/evidence-report.json`: Execution benchmarks, confidence breakdown, and zero-loss audit.

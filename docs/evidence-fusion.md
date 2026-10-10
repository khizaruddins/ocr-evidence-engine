# EDMO Evidence Fusion Engine

## 1. Overview

The **Evidence Fusion Engine** synthesizes multiple heterogeneous signals into a single unified confidence value:
- Original OCR hypothesis & token confidence
- Dictionary verification score
- Fuzzy similarity metric
- Contextual neighbor consistency
- Structural alignment certainty
- Multi-hypothesis OCR passes (e.g. `ocr_primary`, `ocr_secondary`)

---

## 2. Multi-Hypothesis OCR Consensus

When documents undergo multiple OCR passes or ensemble recognition:

```json
{
  "sources": [
    { "source": "ocr_primary", "value": "MATHEMAT1CS", "confidence": 0.82 },
    { "source": "ocr_secondary", "value": "MATHEMATICS", "confidence": 0.94 }
  ]
}
```

The fusion engine calculates token agreement and boosts confidence when independent engines agree on candidate tokens.

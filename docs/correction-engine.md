# EDMO Deterministic OCR Correction Engine

## 1. Overview

The **Deterministic OCR Correction Engine** resolves optical scanning degradations, visual character ambiguities, and typographic confusions without requiring slow or nondeterministic LLM calls.

```
Raw OCR Text
     │
     ├── 1. Exact Dictionary Lookup (General, Academic, Transcript, Grades, Degrees)
     │       └── Found? → Keep original (Confidence: 0.98+)
     │
     ├── 2. OCR Optical Character Confusion Generator
     │       └── Generates candidate variations (e.g. 0↔O, 1↔I, 5↔S, 8↔B, rn↔m, cl↔d)
     │       └── Valid in domain dictionary? → Applied (Method: CHARACTER_CONFUSION_AND_DICTIONARY)
     │
     ├── 3. Fuzzy Normalized Distance Search
     │       └── Evaluates Normalized Levenshtein & Jaro-Winkler across domain vocabularies
     │       └── Safeguard check (Token length >= 3, Similarity >= 0.85)
     │
     └── 4. Domain Context Reinforcement
             └── Column and neighboring field validation
```

---

## 2. Character Confusion Matrix

Models visual OCR confusion pairs with substitution penalties:
- **Digits vs Letters**: `0 ↔ O/o`, `1 ↔ I/l/|`, `2 ↔ Z`, `3 ↔ E`, `4 ↔ A`, `5 ↔ S`, `6 ↔ b`, `8 ↔ B`, `9 ↔ g/q`
- **Ligatures & Split Glyphs**: `rn ↔ m`, `cl ↔ d`, `vv ↔ w`, `li ↔ h`, `ci ↔ a`, `ni ↔ m`

### Safeguards:
- **No Global Blind Regex**: Replacements are validated against domain dictionaries.
- **Short Token Protection**: Acronyms like `CS`, `IN`, `OR`, `A`, `3` are never corrupted.
- **Course Number Invariance**: Course numbers (e.g. `101`, `499`) are protected from numeric-to-letter corruption.

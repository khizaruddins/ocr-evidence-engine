# EDMO Confidence Router

## 1. Decision Logic

The **Confidence Router** controls whether an evidence item is trusted immediately or routed for selective downstream LLM resolution.

```
       Final Fused Confidence (C)
                 │
   ┌─────────────┴─────────────┐
   ▼                           ▼
C >= 0.95                  C < 0.95
(HIGH)                         │
   │               ┌───────────┴───────────┐
   │               ▼                       ▼
   │        0.90 <= C < 0.95            C < 0.90
   │            (MEDIUM)                  (LOW)
   │               │                       │
   │     Deterministic Correction     Deterministic Correction
   │         Boosted >= 0.95?             Boosted >= 0.95?
   │        ┌──────┴──────┐              ┌──────┴──────┐
   │       YES            NO            YES            NO
   │        │              │             │              │
   ▼        ▼              ▼             ▼              ▼
 [TRUST: PRESERVED/    [ROUTED:       [TRUST:        [ROUTED:
    CORRECTED]        LLM_CONTEXT]   CORRECTED]    LLM_CONTEXT]
```

---

## 2. Configurable Thresholds

| Parameter | Default | Description |
|---|---|---|
| `highThreshold` | `0.95` | Score at or above which evidence is trusted without LLM intervention |
| `mediumThreshold` | `0.90` | Boundary below which fields receive aggressive correction attempts |
| `enableLlmRouting` | `true` | When true, ambiguous fields generate tasks in `evidence-llm.json` |

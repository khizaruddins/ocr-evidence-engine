# EDMO Confidence Model Specification

## 1. Multi-Level Confidence Architecture

Confidence is evaluated across multiple structural tiers:
1. **Token Level**: Raw character and word bounding box confidence emitted by the OCR provider.
2. **Block Level**: Weighted mean of constituent token confidences with layout continuity adjustments.
3. **Cell / Row Level**: Column alignment and cell OCR token metrics.
4. **Field Level**: Synthesized multi-factor metric combining raw OCR, semantic label certainty, structural alignment, and contextual coherence.
5. **Document Level**: Aggregated statistical distribution (mean, minimum, maximum, low-confidence token count).

---

## 2. Mathematical Formulation

### Field-Level Confidence Formula

$$\text{Confidence}_{\text{field}} = w_{\text{ocr}} C_{\text{ocr}} + w_{\text{label}} C_{\text{label}} + w_{\text{struct}} C_{\text{struct}} + w_{\text{ctx}} C_{\text{ctx}} - P_{\text{low}}$$

Where:
- $C_{\text{ocr}} \in [0.0, 1.0]$: Mean OCR confidence of the field tokens.
- $C_{\text{label}} \in [0.0, 1.0]$: Confidence of the field key/label identifier.
- $C_{\text{struct}} \in [0.0, 1.0]$: Structural layout consistency (e.g. table cell bounds, line alignment).
- $C_{\text{ctx}} \in [0.0, 1.0]$: Domain context agreement (e.g. valid grade format, credit range).
- $P_{\text{low}}$: Penalty applied when tokens fall below the low-confidence threshold ($\tau_{\text{low}} = 0.80$):

$$P_{\text{low}} = \left(\frac{N_{\text{low}}}{N_{\text{total}}}\right) \cdot \alpha_{\text{penalty}}$$

### Default Weight Configuration:
- $w_{\text{ocr}} = 0.50$
- $w_{\text{label}} = 0.15$
- $w_{\text{struct}} = 0.15$
- $w_{\text{ctx}} = 0.20$
- $\alpha_{\text{penalty}} = 0.25$
- $\tau_{\text{low}} = 0.80$

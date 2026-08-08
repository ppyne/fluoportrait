# Color-science pipeline and assumptions

All math uses `double`; 8-bit quantization occurs only at output. No GUI dependency exists in the core.

## Portrait

Input RGB is treated as IEC 61966-2-1 sRGB. Each normalized channel uses the exact piecewise sRGB inverse transfer function. Linear sRGB is transformed to linear Rec.2020 with the matrix in the product specification. Tiny/out-of-gamut negative channels are clamped to zero, while positive channels are not clamped to one.

The resulting relative-linear Rec.2020 channels are multiplied by `portraitSdrWhiteNits` (75 nits by default). Thus an sRGB white maps to approximately 75 nits per channel. “Approximately” reflects the deliberately fixed, rounded matrix coefficients from the specification; the rows do not sum to precisely one. Each absolute channel is then encoded independently by SMPTE ST 2084 (PQ), whose absolute domain is 0–10,000 nits.

## Direct-PQ background and alpha

`--background-pq R,G,B` values are already 8-bit Rec.2020/PQ code values. They receive no gamut or transfer conversion. For alpha zero the original three bytes are copied exactly into the uncompressed JPEG input buffer.

For partial alpha, both sources must share a linear-light representation. Background PQ channels are decoded to absolute nits; portrait channels are already absolute-linear Rec.2020 nits. Alpha is treated as straight coverage and each channel is blended as `portraitNits*a + backgroundNits*(1-a)`, then PQ-encoded. This avoids sRGB/PQ edge halos. It assumes the PNG stores straight (not premultiplied) alpha, as PNG specifies.

## Luminance and JPEG

Estimated background luminance is `0.2627*R + 0.6780*G + 0.0593*B` after each PQ channel is decoded to nits. JPEG stores the final quantized PQ codes at configurable quality with 4:4:4 sampling. Lossy JPEG may move decoded bytes slightly; uniform direct-PQ backgrounds are tested within a two-code tolerance at quality 100.

The selected ICC bytes are divided into APP2 markers with the `ICC_PROFILE\0` signature, one-based sequence number, and total chunk count. Export validation reconstructs these markers and requires exact equality with the selected profile. Attaching the profile declares the already-produced Rec.2020/PQ pixels; it performs no pixel conversion.

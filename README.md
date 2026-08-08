# fluoportrait

The first increment is a GUI-independent C++20 color engine and command-line exporter. It reads RGBA PNG, converts its assumed-sRGB portrait into SDR-range Rec.2020/PQ, composites an independently specified direct-PQ background, writes 4:4:4 JPEG, embeds a caller-selected ICC profile in standard ICC APP2 chunks, and reopens the JPEG to verify the profile byte-for-byte.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/fluoportrait portrait.png result.jpg \
  --icc "Rec2020 Gamut with PQ Transfer.icc" \
  --background-pq 196,202,156 --sdr-white 300 --quality 100
```

The ICC file is intentionally supplied externally: the project does not redistribute a third-party profile. The exporter attaches it without applying a second color conversion. PNGs carrying a non-sRGB-named ICC profile produce a warning and are treated as sRGB in this version.

See [docs/color-science.md](docs/color-science.md) for the transforms and assumptions.

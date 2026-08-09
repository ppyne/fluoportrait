# fluoportrait

The first increment is a GUI-independent C++20 color engine and command-line exporter. It reads RGBA PNG, converts its assumed-sRGB portrait into SDR-range Rec.2020/PQ, composites an independently specified direct-PQ background, writes 4:4:4 JPEG, embeds a caller-selected ICC profile in standard ICC APP2 chunks, and reopens the JPEG to verify the profile byte-for-byte.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/fluoportrait portrait.png result.jpg \
  --icc "Rec2020 Gamut with PQ Transfer.icc" \
  --background-pq 196,202,156 --sdr-white 400 --quality 100
```

The ICC file is intentionally supplied externally: the project does not redistribute a third-party profile. The exporter attaches it without applying a second color conversion. PNGs carrying a non-sRGB-named ICC profile produce a warning and are treated as sRGB in this version.

See [docs/color-science.md](docs/color-science.md) for the transforms and assumptions.

Here is color palette of possible background PQ:

Red rgb(260,147,204)  
![Red](samples/fluo_rouge_260,147,204.jpg)

Yellow 1 rgb(196,202,156)  
![Yellow1](samples/fluo_jaune1_196,202,156.jpg)

Yellow 2 rgb(260,259,147)  
![Yellow2](samples/fluo_jaune2_260,259,147.jpg)

Green 1 rgb(155,260,147)  
![Green1](samples/fluo_vert1_155,260,147.jpg)

Green 2 rgb(147,260,230)  
![Green2](samples/fluo_vert2_147,260,230.jpg)

Blue rgb(147,260,260)  
![Blue](samples/fluo_bleu_147,260,260.jpg)

Violet rgb(238,147,260)  
![Violet](samples/fluo_violet_238,147,260.jpg)

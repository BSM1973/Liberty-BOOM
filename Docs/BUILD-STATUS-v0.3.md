# BOOM v0.3 — build and quality status

**Verified on 2026-11-16:** source-level standalone C++ DSP test targets only.

| Target | Result | What it proves |
|---|---|---|
| BOOM_DSP | PASS | Simple gain, 3 kHz EQ and compression smoke test |
| BOOM_FACTORY_LIBRARY | PASS | 40 factory styles and 100 uniquely named factory presets |
| BOOM_MASTERING_VALIDATION | PASS | Prototype loudness, correlation, estimated intersample peaks, bypass, limiter guard, high sample rates |
| BOOM_PROCESSOR_REGRESSION | PASS | Independent stereo input trim, wet-path analog output trim, block-size invariance, automated parameter changes |
| GCC AddressSanitizer + UBSan | PASS, 4/4 | No reported sanitizer errors along the tested DSP code paths |
| macOS AU / VST3 | **NOT COMPILED HERE** | Requires macOS build with JUCE 8.0.10 |
| Windows VST3 | **NOT COMPILED HERE** | Requires Windows build with JUCE 8.0.10 |
| DAW host tests | **NOT RUN** | No host validation yet |
| Metering BS.1770 / EBU R128 conformance | **NOT VERIFIED** | Separate measurement tests required |
| True-peak limiter compliance | **NOT VERIFIED** | Failsafe currently only protects sample peaks, estimated TP meter only |
| Anti-aliasing of nonlinear analog stage | **NOT IMPLEMENTED** | Requires proper oversampling and controlled alias tests |

The repository's GitHub Actions workflow is configured to attempt the real plugin builds and upload development artifacts after the sources are pushed into a GitHub repository. Local CI readiness does not imply that those remote builds have run.

## Commands

```sh
cmake -S . -B build-dsp -DBOOM_DSP_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-dsp --parallel 3
ctest --test-dir build-dsp --output-on-failure
```

**Release disclaimer:** BOOM is a development prototype. No real AU or VST3 binaries are included in the source package and it should not yet be marketed as a mastering-grade product.

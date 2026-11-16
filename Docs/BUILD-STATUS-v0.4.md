# BOOM v0.4 — build and validation status

2026-11-16 · Liberty Music Software · development-only sources

## Actually executed in this workspace

| Check | Result | Scope |
|---|---|---|
| CMake `BOOM_DSP_ONLY=ON` Release build | PASS | C++17 Linux/Clang toolchain |
| `BOOM_DSP` | PASS | Core gain, EQ, compression smoke |
| `BOOM_FACTORY_LIBRARY` | PASS | 40 Styles, 100 unique factory presets |
| `BOOM_MASTERING_VALIDATION` | PASS | Loudness prototype, correlation, TP estimate, high-rate DSP |
| `BOOM_PROCESSOR_REGRESSION` | PASS | Stereo trim, wet compensation, block invariance, automation stress |
| `BOOM_V04_DSP_REGRESSION` | PASS | Limiter bypass neutrality, color-mode transitions, nonfinite samples, latency reporting |
| ASan+UBSan (five DSP tests) | PASS | No reported memory errors / UB in paths exercised |

## NOT performed — release blockers

- JUCE 8.0.10 build (not installed; container networking cannot fetch checkout)
- Native AU / VST3 compilation (macOS/Windows runners not available here)
- GUI visual validation for browser, layout, language switching or host automations
- User preset save/rename/delete, `.boom` file dialogs, import/export and DAW-state restore **within a running JUCE plugin**
- Actual DAW playback, mono-monitor offline/real-time behavior, parameter automation, system-wide preset persistence
- True-peak limiter conformance, BS.1770/EBU R128 compliance and licensed real photorealistic artwork
- Oversampled/anti-aliased nonlinear saturation, complete internationalized interface, sign/notarization

The code is a development prototype, not a distribution-ready mastering product. The word "PASS" above applies only to the compiled standalone DSP test executables.

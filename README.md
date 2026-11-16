# BOOM — Professional Analog Mastering Console

Liberty Music Software · **Development prototype v0.5**

> This archive contains source code, not a validated final mastering product.
> Commercial release, LUFS/R128 compliance, true-peak ceiling and DAW behavior
> require specialist measurements and host testing.

## Current implementation

- Native C++17/JUCE 8 stereo processor; AU (macOS) and VST3 (macOS/Windows) target configurations.
- Reference resolution 1440×900; walnut/bronze/champagne console drawing, two moving signal-derived VU meters, 7 main controls, French-first labels, Style/Preset selectors, detailed CHAIN panel.
- Genuine DSP: input drive plus independent L/R trims (±12 dB), original analog-style nonlinearities (Clean/Tube/Tape) with wet-path output compensation, 5-band EQ, stereo-linked high-frequency de-esser with adjustable attack/release, glue compressor, peak limiter with a 5 ms delay and output trim. TUBE mode now uses 4x oversampling (65-tap FIR interpolation and reconstruction); all color paths are aligned to the added 16-sample delay.
- `PUSH` dynamically changes saturation drive, compressor settings, limiting and gain. Its behavior is an early prototype, not acoustically calibrated.
- Per-module bypass with smooth crossfades: color, EQ, de-esser, compressor and limiter; compressor stereo linking can be switched on/off. CHAIN includes sound-affecting color and limiter mode selectors.
- Metering: 300 ms VU, peaks, RMS, compressor/limiter gain reduction, K-weighted 400 ms and 3 s loudness, relative-gated integrated loudness, approximate LRA, correlation and 4× oversampled true-peak estimate. LUFS/LRA and true-peak need independent BS.1770/EBU R128 conformance tests.
- 40 Style templates and 100 unique factory preset configurations in ten categories. These settings are systematically constructed and **have not been auditioned or validated by professional mastering engineers**.
- A/B parameter snapshots, A→B / B→A copying and browser favorites are saved into the DAW project state. Closing and reopening the plugin editor no longer clears the A/B states.
- User preset browser: factory/user filter, search, save/rename/delete (two-click confirmation), recall, and versioned `.boom` XML import/export. User presets are embedded in DAW project state, rather than silently stored in a global library; export is required for sharing between projects. The UI/browser and host serialization have **not yet been compiled or tested in JUCE**.
- 12 ms smoothing for dynamics thresholds, amounts and mixes, and a 25 ms crossfade on analog character mode changes. The limiter-bypass path now disables its internal predrive as well as gain reduction.
- When the host reports an offline bounce through `isNonRealtime()`, the monitoring-only MONO option is suppressed. Real-time capture may still record mono and must be tested in each host.
- Audio callback has no explicit dynamic allocation, locks, filesystem activity or UI calls. The meter's history buffers and delay line allocate during `prepareToPlay()`.

## Changes in v0.5 (16 November 2026)

- Added `Source/OversampledTube.h`: an original 4x polyphase FIR upsampling/downsampling path around the TUBE waveshaper. Fixed-size buffers and filters are prepared outside the audio callback; not a hardware-modeling claim.
- Added a 16-sample color-stage delay to CLEAN, TAPE, dry color, and global bypass paths so that all routes remain time aligned. Total reported plugin latency is `ceil(sampleRate * 0.005) + 16` samples (256 at 48 kHz).
- A 17 kHz / 0.7-amplitude test tone at 48 kHz yielded approximately **73 dB** less 3 kHz alias component than the original non-oversampled TUBE waveshaper. This single-tone result does **not** guarantee anti-aliasing effectiveness for arbitrary material.
- Added a conservative **0.10 dB guard margin** inside the output limiter. Independent FFmpeg loudnorm analysis of an aggressive 48 kHz stereo test render at a requested -1.0 dBTP ceiling measured **-1.09 dBTP**, versus **-0.99 dBTP** before the guard. This is **not a conformance certificate** and the limiter must still be tested under multiple frequencies, signals, output trims and overshoot conditions.
- Increased the LRA histogram sampling frequency from 1 second to 100 ms. On the supplied simple 48 kHz, 1 kHz stereo reference tone (0.1 peak, 8 seconds signal + 3 seconds silence), FFmpeg ebur128 measured integrated loudness **-20.1 LUFS**, LRA **5.7 LU**. BOOM measured **-20.12 LUFS**, LRA **5.8 LU**. Validation at 44.1, 96 and 192 kHz demonstrated similar measurements, but did not compare against an external instrument at those rates.
- Added two DSP regression suites: oversampling anti-aliasing/latency and loudness-reference sample-rate/gating. The standalone offline true-peak audio probe can be regenerated using the instructions in `Docs/VALIDATION-v0.5.md`.
- Enabled GitHub Actions push triggers for feature branches (in addition to `workflow_dispatch` and PRs). Native AU/VST3 configuration remains **unverified** until the workflow is actually run on GitHub runners.
- Linux Release and ASan/UBSan: **7/7 test suites pass**. Native JUCE/macos/Windows plugin compilation has not run here.

## Changes in v0.4

- Added user preset snapshot management with up to 512 slots, project-state serialization and portable `.boom` import/export. Factory bank remains unchanged at 100 presets.
- Rebuilt the preset browser as a larger temporary overlay with factory/user categories, name editing, two-click delete confirmation, and file dialogs.
- Excluded UI language, VU input selection and monitoring-only mono from user preset and A/B recalls to avoid surprising monitoring changes.
- Fixed the audio-engine bug where bypassed limiting still boosted the signal through limiter predrive.
- Added dezippering for dynamics and mix parameters, smooth transitions when CLEAN/TUBE/TAPE changes, and finite-sample input guards.
- Added a fifth C++ regression suite testing limiter-bypass neutrality, color-morph continuity, invalid samples and 44.1–192 kHz latency reporting.
- Linux C++ Release and ASan/UBSan: all seven DSP suites passed. Native JUCE / AU / VST3 builds and UI tests remain unverified.

## Previous changes in v0.3

- Fixed missing atomic LUFS/LRA/true-peak/correlation members in `PluginProcessor.h`; v0.2 referenced these in its implementation, which would fail compilation against JUCE.
- Added DAW-automatable L/R input trim (±12 dB), Analog Color wet-only output compensation (±12 dB), and de-esser attack (0.1–20 ms) / release (20–500 ms). These parameters are visible in the CHAIN modules and influence real DSP.
- Changed the initial Style/Preset selector to unselected: a default flat state must not misleadingly claim that a factory preset was loaded. Hosts still see program index 0 until a preset is selected.
- Added A/B copy commands through a main-console menu. These copy whole stored state snapshots; level matching still needs implementation.
- Respaced the five main EQ knobs to remove their five-pixel overlapping component bounds.
- Added a fourth dedicated DSP regression executable with stereo-trim, analog compensation, variable buffer size and aggressive parameter automation tests. Four Release tests and all four tests under AddressSanitizer/UndefinedBehaviorSanitizer passed in the Linux tool environment.
- CI now includes a sanitizer-validation job as well as the native plugin build attempts.

## Build native plugins

The CMake build downloads JUCE 8.0.10 from GitHub unless you pass a locally installed checkout with `-DJUCE_SOURCE_DIR=/path/to/JUCE`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 3
ctest --test-dir build -C Release --output-on-failure
```

On macOS, AU/VST3/Standalone targets are enabled. On Windows, VST3/Standalone. The GitHub workflow attempts macOS ARM64, macOS Intel and Windows x64 builds and uploads unsigned artifacts when a repository contains these sources. **This local archive has NOT been compiled against JUCE or inside a DAW**, because the local environment has no JUCE checkout or network access.

## Standalone DSP validation (no JUCE checkout required)

```sh
cmake -S . -B build-dsp -DBOOM_DSP_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-dsp --parallel 3
ctest --test-dir build-dsp --output-on-failure
```

Tests cover gain/EQ, preset count/naming, LF/peak/loudness gate, 4× intersample interpolation, per-module bypass, limiter sample-peak guard, sample-rate stability through 192 kHz, stereo trims, analog wet gain compensation, buffer invariance and automated parameter stress. Local Release and AddressSanitizer/UndefinedBehaviorSanitizer validation (7/7 tests) passed on Linux. These do not substitute for actual JUCE/AU/VST3 compilation or host testing.

## Critical work remaining

1. Compile and open AU and VST3 in real DAWs (Logic, Live, Bitwig, Studio One, Cubase, REAPER).
2. Replace the prototype true-peak limiter with a benchmarked BS.1770 / IEC-compliant algorithm and formal TP ceiling tests. The existing limiter has a conservative 0.10 dB ceiling margin, but it is still a prototype and needs formal true-peak compliance measurement.
3. Validate loudness gating/LRA at all sample rates against reference audio and a certified meter.
4. Add proper oversampling/anti-aliasing to nonlinear analog stages, characterize aliasing/THD, optimize CPU.
5. Compile and validate the new user preset browser, host state serialization, XML file import/export and round-trip within actual JUCE plugins; add a separately persistent global user library and global favourites across DAW projects if needed.
6. Add missing advanced parameters, automation smoothing for remaining controls, and more thorough transport, automation and state tests.
7. Create licensed/owned photorealistic studio and hardware artwork: existing artwork is procedural JUCE graphics, not photorealistic imagery.
8. Add complete UI localization and assess accessibility, sizing and text legibility in the actual plugins.
9. Validate A/B level matching and longer running, format-specific audio tests. Automatic level matching is not included yet.
10. Code-sign/notarize macOS packages and prepare an installer when functionality is verified.

Original technical specification is in `Docs/BOOM-Cahier-des-charges.txt`. The supplied specification ends midway through section 19; no missing text has been invented.

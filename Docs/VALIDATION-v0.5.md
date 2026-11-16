# BOOM v0.5 — DSP validation report

Date: 2026-11-16. Local Linux C++ validation; **native macOS AU/VST3 NOT compiled**.

## Commands

```sh
cmake -S . -B build-dsp -DBOOM_DSP_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-dsp --parallel 4
ctest --test-dir build-dsp --output-on-failure

./build-dsp/boom_v05_true_peak_probe /tmp/boom-probe.f32
ffmpeg -hide_banner -nostats -f f32le -ar 48000 -ac 2 -i /tmp/boom-probe.f32 -af 'loudnorm=I=-14:TP=-1:LRA=11:print_format=json' -f null -
```

For native macOS AU/VST3, create/import this source tree in a GitHub repository and run `.github/workflows/build.yml` (Actions > `BOOM - native plugins and DSP tests` > Run workflow). Inspect every native build job and artifact rather than assuming the workflow succeeds.

## Executed measurements

| Check | Observed | Interpretation |
| --- | --- | --- |
| DSP Release | 7/7 test suites pass | Partial unit/regression validation, not host validation |
| AddressSanitizer/UndefinedBehaviorSanitizer | 7/7 test suites pass | No errors observed on exercised code paths |
| 4x TUBE alias probe | 17 kHz input, folded third harmonic at 3 kHz, about -73 dB vs naive baseline | Single-tone evidence of anti-aliasing, not full spectrum assessment |
| Latency at 48 kHz | 256 samples (240 lookahead + 16 fixed color FIR) | Must be reported to host; native implementation not checked |
| LUFS integrated after 8 s sine + 3 s silence, 48 kHz | BOOM -20.12 LUFS, FFmpeg ebur128 -20.1 LUFS | Agreement for this fixture; does not prove BS.1770 / EBU R128 compliance |
| LRA for same fixture | BOOM 5.8 LU, FFmpeg ebur128 5.7 LU | Agreement for this fixture; more program material needed |
| True-peak ceiling -1.0 dBTP (aggressive synthetic mix) | FFmpeg loudnorm input_tp = -1.09 dBTP (after 0.1 dB guard) | Probe passes target; not a universal ceiling guarantee |

## Limitations

- True-peak detector and safety stage remain a prototype, not a validated intersample-peak ceiling algorithm across all content.
- LUFS K-filter, gating and LRA need more than one reference: speech, variable-level music, silence, DC, correlated/uncorrelated surround cases where applicable, all sample rates, long-term recordings, and approved conformance fixtures.
- Tube oversampling CPU load, noise floor, THD and all alias products have not been benchmarked under real-time DAW stress.
- TAPE mode is not yet oversampled; the TUBE path alone is 4x.
- Plugin binaries cannot be built without JUCE and a macOS toolchain. This environment had neither JUCE nor access to github.com for CMake FetchContent.
- No AU/VST3 native UI, serialization, host automation or DAW compatibility validation has been performed here.
- No signed/notarized installer or commercial release is included.

# New DAW source-tree integration

Canonical base: the user-provided `wezt-source(1).zip` source tree.

## Integrated

1. **Production DSP inserts:** added Wezt Saturator, Wezt De-Esser, Wezt Stereo Delay and Wezt Reverb. All are built-in `IPluginInstance` implementations with real parameter tables, state serialization, automation support and session factory registration.
2. **Presets:** added 17 genre profiles across all eight built-in effects (136 combinations), including Dancehall and Honduran Raspe. Preset changes use the existing validated parameter/undo service.
3. **AI/plugin workflow:** added `apply_genre_plugin_preset` to the PluginAgent and MCP. The DAW's existing multi-track mix analyzer continues to measure track clashes and stage EQ fixes for explicit application; plugin parameter/tune/undo tools remain the common control surface.
4. **Desktop UI:** added genre preset selection/application to Plugin Parameters; the Mix window now offers built-in effect insertion and opens the selected insert's parameter window. Existing automation controls remain in place.
5. **Verification:** expanded tests for effect creation and signal behavior, genre/effect preset coverage, MCP registration and session save/load restoration.

## Local verification

- CMake Release configuration: passed.
- Full default headless Release build: passed (resumed after environment command timeouts).
- All 28 tests: passed. Tests were run in batches/individually because the command runner's 120-second execution window expired during the longest test batch.
- `wezt_agent_test`: passed after final preset/session coverage additions.
- `wezt_mcp_test`: passed.
- Windows/macOS simulator tests: passed as part of the local suite; these are simulations, not native platform builds.

## Not yet verified

- The ImGui/GLFW desktop UI was edited but not compiled or visually exercised because Dear ImGui and GLFW development sources/libraries are absent in this environment.
- This integration revision has not yet been built on native Windows or macOS CI runners.
- Actual commercial VST3/CLAP plugin behavior and real audio hardware are not verified by this headless build.
- Genre presets are starting points, not guaranteed radio-ready processing. Coordinated measured mix analysis remains an explicit analyze/review/apply workflow.


## Next engineering priority: desktop CI verification hardening

- The three-platform app workflow now requires the `wezt-studio` desktop executable to exist; a configuration that silently omits the actual app fails the job.
- UI snapshots are rendered without `|| true`, so renderer crashes and nonzero exits fail CI.
- Ten required views are captured: Edit, Mix, Sampler, Assistant, Piano Roll, Clips, Immersive, Settings, Plugin Parameters, and an interactive plugin-parameter graph render.
- Each PPM is checked for its exact 1600x940 P6 header, full pixel payload, and nonblank content before upload.
- The desktop executable artifact is now required rather than uploaded with `if-no-files-found: warn`.
- Local Release headless build completed and all 28/28 tests passed after resuming timed-out build commands.
- This environment has no Dear ImGui or GLFW development installation, so the actual desktop UI build and screenshots remain pending execution by the configured GitHub Actions app matrix. The workflow changes are prepared but have not been run remotely in this session.

## Next engineering priority: real-time DSP stress and parameter handoff

- Added `wezt_realtime_dsp_test`: a prepared eight-insert audio graph rendered for 6,000 callbacks across 64/96/128/256/512/1024-frame blocks.
- The test tracks regular and aligned heap allocations on the render thread and reports zero allocations during processing.
- A concurrent control thread updates compressor, limiter and EQ parameters while audio is rendering; all samples remain finite and callback frame counts remain correct.
- Compressor and limiter parameter fields now use atomic scalar handoff; gain-reduction telemetry is atomic as well.
- Replaced the limiter's O(lookahead samples) per-sample window scan with a fixed-capacity monotonic queue allocated in `prepare()`, making minimum tracking amortized O(1) per sample.
- Release run observed approximately 45 microseconds average and 160 microseconds p99 callback time on this host; timings are diagnostic and not a hardware real-time guarantee.
- Full Release build completed. All 29 CTest tests passed across two batches because the command execution window expired during the first full-suite invocation; the remaining three tests passed in a separate run.
- The targeted Linux ThreadSanitizer build and `wezt_realtime_dsp_test` passed without a race report. The ASan/UBSan build was attempted but did not complete within the available execution window, so it remains pending. Native Windows/macOS builds are still pending actual CI runners; simulator tests are not reported as native OS verification.

## AddressSanitizer/UndefinedBehaviorSanitizer verification

- Completed a focused GCC 14.2 build of `wezt_realtime_dsp_test` with both ASan and UBSan enabled.
- Ran 6,000 callbacks through the eight-effect chain with leak detection and halt-on-error enabled; no sanitizer or leak report occurred.
- The test verified finite output, zero audio-thread allocations and concurrent compressor/limiter/EQ parameter updates.
- Adjusted the reproducible `asan` CMake preset to use `-O1 -g0` in RelWithDebInfo, reducing build time in constrained local environments.
- This is a targeted sanitizer pass, not a claim that every test target has been run under sanitizers.

## Next engineering priority: SDK-backed real plugin validation

- The source already contains VST3 and CLAP host test targets and a three-platform plugin CI matrix.
- This local environment does not contain the Steinberg VST3 SDK or CLAP headers.
- Fetching the CLAP repository was attempted, but GitHub DNS resolution failed (`Could not resolve host: github.com`).
- Therefore real SDK-backed VST3/CLAP plugin compilation and runtime testing remain pending an SDK-enabled CI runner; simulator tests are not substituted for that evidence.


## VST3 / CLAP integration hardening

- CLAP sample-offset MIDI ordering now uses stable, bounded insertion sort rather than
  `std::stable_sort`, avoiding a possible temporary heap allocation on the audio thread.
- CLAP integration test now exercises concurrent module lifetime: scan the module while an instance
  is open, open a second instance, destroy it, and confirm the first instance remains valid. It also
  verifies a missing plugin ID is rejected without damaging the live instance.
- VST3/CLAP test executables use exit code 77 when invoked without a real plugin; CTest reports this
  as skipped rather than green. SDK-enabled CI remains responsible for running the real binary tests.
- This source-level hardening is not a substitute for the SDK-backed VST3/CLAP build. The current
  local environment still has no Steinberg VST3 SDK or CLAP headers, and remote CI has not been
  observed from this session.

### Local regression verification after integration hardening

- Release configure and full build completed successfully with GCC 14.2 / Ninja.
- Release CTest suite: **29/29 passed** (16.29 seconds).
- ASan/UBSan targeted regression checks: **2/2 passed** (`wezt_tests` and `wezt_realtime_dsp_test`); the real-time stress target completed without sanitizer or leak reports.
- These local runs do not compile the optional CLAP/VST3 host targets because their external SDK/header dependencies are absent in this environment. The dedicated CI workflow now asserts the four real plugin binaries exist before running its plugin test suite; that remote matrix has not yet been observed.

## Real VST3/CLAP integration gate hardening

- Fixed the VST3 no-sample-plugin CTest invocation: it now launches the test with no plugin argument,
  allowing the executable to return CTest's explicit skip code 77 instead of receiving an empty path
  and failing for the wrong reason.
- The dedicated cross-platform plugin workflow now emits a CTest JUnit report and rejects missing,
  skipped, or failed real integration cases. It requires the VST3 effect, four VST3 instrument MIDI
  cases, CLAP gain effect, and CLAP MIDI instrument cases.
- Updated the format capability table to distinguish implemented test coverage and configured CI
  from actual observed cross-platform test passes. No SDK-backed result is claimed until CI runs.

- Removed the remaining `std::stable_sort` from the VST3 audio callback and replaced it with the same bounded stable insertion sort used by CLAP.
- Increased CLAP/VST3 event-list and VST3 parameter-change preallocation to cover the maximum incoming MIDI block, queued parameter changes, and the worst-case 128-key x 16-channel AllNotesOff expansion.

## SDK-backed VST3/CLAP gate preparation (2026-11-26)

- Fixed VST3 sample bundle paths to use `$<CONFIG>` so multi-configuration generators (notably Visual Studio) select the correct bundle directory.
- Kept explicit build dependencies for the VST3 effect and MDA instrument sample bundles.
- Narrowed the dedicated plugin workflow to exactly the seven real integration tests and strengthened the JUnit gate to require exactly seven cases, no skips, and no failures.
- SDK-independent Release build completed and CTest passed 29/29 in this environment.
- Official VST3 SDK and CLAP headers are not present locally; GitHub DNS resolution failed. Consequently, no real SDK build or Linux/Windows/macOS integration result is claimed. The three-platform matrix remains pending execution in a network-enabled CI runner.

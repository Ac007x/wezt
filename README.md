# Wezt Recording Tools

An open-source, clean-room digital audio workstation for Windows and macOS, aiming at the
multitrack recording, editing and mixing workflow of professional DAWs plus the creative
tooling Logic Pro users expect.

Wezt Recording Tools is an independent project. It contains no Avid or Apple code, assets,
names, icons or UI text and is not affiliated with either company. See
`docs/CLEAN_ROOM_POLICY.md`.

## v0.7: music engine, mix assistant, tutor and Claude connector (docs/CLAUDE_CONNECTOR.md)

## Status (v0.6)

"Simulated" means the real backend source runs against our own model of the Windows or macOS
API on Linux (`tests/sim/`). That checks the logic, not the real OS: expect small fixes on first
contact with real Windows and macOS. Nothing here has yet been built on either.

| Area | State |
|---|---|
| Engine, graph, transport (play/stop/record/locate/loop), recording to BWF/WAV | works, tested |
| Audio I/O: WASAPI (Windows) | shared and **exclusive** mode, device picker, MMCSS "Pro Audio" priority, 24-bit-in-32 and other native formats, alignment retry, fallback to shared. **Simulated** |
| Audio I/O: CoreAudio (macOS) | device picker, separate input/output devices through an aggregate device, **hog mode** (exclusive), latency report. **Simulated** |
| Session and mixer: tracks, clips with fades, fader/pan/mute/solo, inserts, sends, aux buses, master, metering, delay compensation, versioned save/load, offline bounce | works, tested (headless) |
| VST3 hosting | works, tested with a real plugin |
| CLAP hosting | works, tested with a real plugin |
| 32-bit bridge | works with a same-bitness helper, output identical to native; survives plugin crashes, hangs and host death (tested); real 32-bit helper untested |
| Plugin scanner and installer for every format | works, tested |
| AU, VST2 | scanned, no loader yet |
| AAX, RTAS, TDM | not hostable; listed and matched to VST3/CLAP/AU twins (`docs/PLUGIN_FORMATS.md`) |
| User interface (Dear ImGui, C++): transport, Edit and Mix windows, tools, Pro Tools key map, +/-/duplicate track | logic and layout tested headless; the GLFW window is untested here (`docs/UI.md`) |
| Sampler instrument (zones, loops, round robin, filter, LFO, slicing, patches) | works, tested; built in as an insert |
| Immersive: speaker layouts, VBAP object renderer, binaural monitor, BS.1770 loudness, ADM BWF export | works, tested; not routed into live audio yet (`docs/IMMERSIVE.md`) |
| Editing layer: undo/redo, memory locations, strip silence, tab to transients, clip gain envelopes, automation modes (Touch/Latch/Write), thin automation, VCA masters | works, tested (`docs/FEATURE_COVERAGE.md` says what is wired to the UI) |
| Local assistant: measures audio, learns your style and reference profiles, advises, masters (EQ, compressor, limiter), optional loopback-only local LLM | works, tested; no cloud (`docs/AI.md`) |
| AAF/OMF import/export, MIDI, Elastic Audio, Stem Separator | not started (`docs/ROADMAP.md`) |

### Exclusive mode

- **Windows:** Wezt opens the device in WASAPI exclusive mode when asked, and falls back to
  shared mode with a notice if it cannot. The two Sound-settings switches, "Allow applications
  to take exclusive control of this device" and "Give exclusive mode applications priority", are
  per-device user settings. Wezt reads and reports them but never changes them; if exclusive mode
  is blocked, the error says which switch to turn on.
- **macOS:** exclusive access is CoreAudio "hog mode". Wezt never takes a device another process
  already owns, and releases it on stop.

## Build

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Requires a C++20 compiler (MSVC 2022, Clang 15+, GCC 12+) and CMake 3.20+. VST3, CLAP and the
bridge are optional and need their SDKs (VST3 needs CMake 3.25+); see `docs/PLUGIN_FORMATS.md`.

## Using the engine

```cpp
Graph g;
auto src = g.add(std::make_unique<Sine>(440.0));
g.setOutput(src);
Engine engine(g, /*block*/256, /*channels*/2, /*rate*/48000.0);

auto dev = audio::createPlatformDevice();   // WASAPI or CoreAudio
audio::AudioConfig cfg; std::string err;
dev->open(cfg, &engine, &err);
dev->start(&err);
engine.transport().play();
```

## Layout

```
docs/        architecture, plugin formats, UI, immersive audio, roadmap, clean-room policy
src/engine/  graph, transport, engine driver (no third-party deps)
src/audio/   device interface; WASAPI, CoreAudio, null backends, sample-format conversion
src/io/      WAV/BWF/RF64 reader and writer
src/session/ mixer strips, clips, session save/load, bounce
src/instruments/ sampler
src/immersive/ layouts, VBAP renderer, binaural, loudness, ADM export
src/ui/      commands and key map, the Dear ImGui application
tools/       CPU screenshot tool for the UI
src/plugins/ VST3 + CLAP hosts, plugin library (scan/install), 32-bit bridge, AAX twin finder
tests/       unit tests; Windows/macOS simulators (tests/sim); plugin host tests
```

Plugin control for AI agents (parameters, intent tuning, CLAP `wezt.agent/1`): docs/AGENT_INTERFACE.md

## Build presets and CI

`CMakePresets.json` has `release`, `debug`, `clang`, `asan` (AddressSanitizer + UBSan) and `tsan`:

```
cmake --preset asan && cmake --build --preset asan && ctest --preset asan
```

`.github/workflows/ci.yml` runs those four presets on Linux, plus experimental Windows (MSVC) and macOS jobs.
The Windows and macOS jobs are marked `continue-on-error` because the WASAPI and CoreAudio backends have never been
built on those systems; remove that flag once they are green. MSVC gets `_USE_MATH_DEFINES`, `NOMINMAX` and `/utf-8`.

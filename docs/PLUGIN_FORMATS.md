# Plugin Formats

What Wezt Recording Tools can do with each format, as of this build. "Tested" means a test in
`tests/` runs it; it does not mean it has been run on every OS.

| Format | Windows | macOS | Status |
|---|---|---|---|
| VST3 | yes | yes | Host implementation and real-plugin integration tests are present. The SDK-backed Linux/Windows/macOS CI matrix is configured, but a run must pass before cross-platform real-plugin validation can be claimed. Third-party commercial plug-ins are untested. |
| CLAP | yes | yes | Host implementation and real-binary integration tests are present, including note-event and raw-MIDI synth cases. The Linux/Windows/macOS CI matrix is configured, but a run must pass before cross-platform real-plugin validation can be claimed. |
| 32-bit plugins | yes | no | **Bridge built and tested** with a same-bitness helper: output is bit-identical to running in-process, and a crashed helper yields silence instead of taking the session down. A real 32-bit helper has **not** been built or run. |
| AU | no | yes | Found by the scanner; **no loader yet**. |
| VST2 | yes | yes | Found by the scanner; **no loader yet**. Steinberg no longer licenses the VST2 SDK, so a loader needs your own headers. |
| AAX | yes | yes | **Not hostable.** Found, listed, and matched to a hostable twin. |
| RTAS, TDM | yes | yes | **Not hostable.** Found, listed, and explained. |

## Building the hosts

```
git clone --recursive https://github.com/steinbergmedia/vst3sdk
git clone https://github.com/free-audio/clap
cmake -S . -B build -DWEZT_ENABLE_VST3=ON -DWEZT_VST3_SDK_DIR=<vst3sdk> \
                    -DWEZT_ENABLE_CLAP=ON -DWEZT_CLAP_DIR=<clap> -DWEZT_ENABLE_BRIDGE=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Both SDKs are MIT licensed. The VST3 SDK needs CMake 3.25+. To run the plugin tests against a
real VST3, add `-DSMTG_ENABLE_VST3_PLUGIN_EXAMPLES=ON` and
`-DWEZT_VST3_TEST_PLUGIN=<path to again-sample-accurate.vst3>`.

MIDI note, controller and pitch-bend events are supported in both hosts, so VST3 and CLAP instruments work
(`acceptsMidi()` / `handleMidi()`; the built-in Sampler and Wezt Synth use the same path). Not supported yet in the
hosts: side-chain buses (extra input ports are fed silence) and plug-in editor windows (parameters are edited in
Wezt's own Plugin Parameters window).

## "Every plugin installed and found" (`PluginLibrary`)

`PluginLibrary::scan()` looks in the standard system and per-user folders for every format on
the current OS and reports, for each plugin: its format, its bitness (read from the file header,
never by loading it), and one of four verdicts with a plain-language reason:

- **Native**: loads in-process.
- **NeedsBridge**: a 32-bit plugin; runs through the bridge on Windows.
- **NotBuilt**: a format we intend to host, but this build lacks the loader.
- **NotHostable**: Pro Tools-only formats.

`PluginLibrary::install()` copies a plugin file or bundle into the per-user folder for its
format. It does not run vendor installers (`.exe`, `.msi`, `.pkg`) and refuses Pro Tools-only
formats, saying so.

## The 32-bit bridge

A 64-bit process cannot load a 32-bit plugin, so a small helper (`wezt-bridge`), built with the
plugin's bitness, hosts it. Audio, parameters and state travel over shared memory. A block that
takes too long is replaced by silence and counted, so a hung plugin cannot stall the audio thread.

**It does not make plugins sound richer.** The bridge passes 32-bit float audio through
untouched, and the test confirms the output is identical (difference 0) to the plugin run
natively. Running a plugin "in 64-bit" changes its address space, not its sound. Whatever
character a 32-bit plugin has, it keeps. Processing that does change the sound (oversampling,
saturation) would be an explicit effect, and would be audible as such.

To use one on Windows: build `wezt-bridge` as 32-bit (`cmake -A Win32`) and set
`BridgeConfig::helperExecutable` to it. macOS dropped 32-bit support in 10.15, so there is no
32-bit path there.

## AAX, RTAS and TDM

These cannot be hosted by a third-party program, and that is a licensing and technical fact, not
a gap more engineering would close.

- **AAX** plugins are built against Avid's AAX SDK for Pro Tools' own host interfaces. Avid
  licenses that SDK to plugin makers (with PACE signing) and publishes no host-side SDK.
- **RTAS** is a retired Pro Tools format: 32-bit, and tied to Pro Tools' internal plugin
  interfaces.
- **TDM** runs on Avid HD DSP cards, hardware a software host does not have.

Hosting any of them would mean emulating Pro Tools' interfaces by reverse engineering Avid's
software, which this project's clean-room policy and Avid's licence rule out. RTAS and TDM share
the `.dpm` extension, so the scanner reports them as one "RTAS/TDM" kind without opening the file.

What Wezt does instead: it lists them, then looks for the same plugin installed as VST3, CLAP or
AU (preferring VST3) and offers that. Most commercial plugins ship in several formats from one
codebase, so a hostable twin is usually already on the machine. The only legitimate route to
real AAX hosting is a licence from Avid.


## Real-time MIDI ordering hardening

CLAP and VST3 MIDI events are kept in sample-offset order using bounded, stable in-place insertion
sorts. Both hosts deliberately avoid `std::stable_sort` in their process callbacks because standard-
library implementations may allocate a temporary buffer. Equal-offset events retain their incoming
order. CLAP event storage reserves capacity for the maximum parameter-event, incoming-MIDI, and
expanded AllNotesOff burst; VST3 event and parameter-change lists likewise reserve their bounded
worst-case capacity during instance construction. These are source-level safeguards; a real SDK-backed
allocation-instrumented plugin run remains part of the pending integration validation.

Integration tests that lack a real plugin binary return CTest's configured skip code (77), rather
than returning success. The SDK-enabled CI job supplies the Steinberg sample modules and builds the
project's CLAP test modules, so those jobs exercise real plugin binaries.

The dedicated plugin CI matrix asserts that its four real test modules exist after the build
(VST3 effect, VST3 instrument bundle, CLAP gain effect, and CLAP MIDI instrument). It then parses
CTest's JUnit report and requires all seven real integration cases (the VST3 effect, four VST3
instrument cases, and two CLAP cases) to be present with no skips or failures. This prevents a
misconfigured SDK build from appearing healthy merely because a host test had no binary to load.


## Real SDK integration gate

The SDK-backed integration matrix is defined in `.github/workflows/ci.yml` and runs on Linux, Windows, and macOS. It builds the Steinberg SDK sample effect (`again-sample-accurate.vst3`), the MDA instrument bundle (`mda-vst3.vst3`), and the two project-owned CLAP test modules. The dedicated gate runs exactly seven tests: one VST3 effect/control/state test, four VST3 MIDI instrument tests, one CLAP effect/control/state test, and one CLAP MIDI/note-event test. The workflow rejects missing bundles, missing tests, skipped tests, and failed tests.

VST3 bundle paths use CMake's `$<CONFIG>` generator expression so Visual Studio multi-configuration builds resolve the correct `VST3/Release` (or selected configuration) directory; `CMAKE_BUILD_TYPE` is not reliable for multi-config generators.

**Verification status:** the SDK-independent Release suite has passed 29/29 tests in the current working environment. The SDK-backed matrix has not been executed in this environment: the official VST3 SDK and CLAP headers are not installed, and GitHub DNS/network access is unavailable. Do not interpret the presence of the CI workflow as evidence that real VST3/CLAP integration has passed. Completion requires successful workflow runs on all three operating systems with seven tests, zero skips, and zero failures on each.

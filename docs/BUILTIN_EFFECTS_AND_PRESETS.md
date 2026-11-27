# Built-in effects and genre starting points

The built-in effects are ordinary `IPluginInstance` inserts. They use the same session serialization,
parameter inspection/editing, automation lanes, undo history, plugin-agent service and MCP control
surface as third-party plug-ins.

## Effects

| Effect | Controls | Purpose |
|---|---|---|
| Wezt EQ | Four bands: low shelf, two parametric bells, high shelf | Broad tonal shaping |
| Wezt Compressor | Threshold, ratio, attack, release, knee, makeup, mix | Dynamic control |
| Wezt Limiter | Ceiling, release; reports look-ahead latency | Peak control |
| Wezt Stereo Width | Mid/side width | Stereo image adjustment |
| Wezt Saturator | Drive, output, mix | Harmonic saturation |
| Wezt De-Esser | Detector frequency, threshold, reduction | Sibilance control |
| Wezt Stereo Delay | Time, feedback, mix, ping-pong | Tempo/time-based echo |
| Wezt Reverb | Room, damping, mix | Stereo ambience |

All delay buffers are allocated in `prepare`, not in the audio `process` callback. The effects are
built into the native factory, so session load/duplicate recreates the same classes from the saved
built-in descriptor and restores parameter state.

## Genre presets

The Plugin Parameters window provides a genre selector and **Apply genre preset** action for built-in
effects. The same operation is available to agents through the MCP tool
`apply_genre_plugin_preset(track, plugin, genre)`. Each operation is checked against the target
plugin's real parameter names/ranges and is undoable.

Supported profiles: Pop, Drill, Hip Hop, R&B & Soul, Reggae, Dancehall, Soca, Funk Carioca,
Reggaeton, Rock, Heavy Metal, Punk, Punta, Dubstep, Honduran Raspe, Afrobeats and Other.

The starting points use conservative values. They do not analyze the user's actual recording by
themselves and do not guarantee a radio-ready mix or master. The existing mix analyzer measures
tracks together, reports frequency clashes and proposes corrective EQ; the user/agent must explicitly
apply those fixes. Mastering remains a separate measured render workflow.

## Verification

- All 17 genre profiles have a settings map for all 8 built-in effects (136 effect/profile pairs).
- New processors have signal-level smoke tests (saturation, sibilance reduction, delay timing and
  reverb tail), parameter/state tests and session save/load coverage.
- `wezt_agent_test` exercises built-in creation, preset application, unsupported-preset rejection,
  undo, MCP discovery and the genre catalogue.
- The Linux Release headless build and 28-test suite pass. The desktop UI additions still need a
  dependency-enabled ImGui/GLFW build. Windows/macOS CI has not yet run this integration revision.

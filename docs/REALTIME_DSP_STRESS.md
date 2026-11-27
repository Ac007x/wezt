# Real-time DSP stress validation

The `wezt_realtime_dsp_test` target exercises the prepared audio graph with this eight-insert chain: Parametric EQ, Compressor, Saturator, De-Esser, Stereo Delay, Reverb, Stereo Width and Look-ahead Limiter.

It renders 6,000 callbacks using 64, 96, 128, 256, 512 and 1,024 frame blocks at 48 kHz. During rendering, a separate control thread continuously changes compressor, limiter and EQ parameters. The test verifies output frame counts, finite output samples and zero heap allocations on the render thread, including aligned allocations.

The test prints average, 99th-percentile and maximum callback duration. These timings are diagnostic only: shared CI runners and virtualized hosts have scheduling jitter, so the test does not assert a machine-specific real-time deadline. This is not a substitute for testing on the user's audio hardware and target buffer configuration.

## Limiter and parameter handoff

The look-ahead limiter now uses a fixed-capacity monotonic minimum queue allocated in `prepare()`. This changes per-sample look-ahead minimum tracking from a full-window scan to amortized O(1) queue operations while retaining the L+1 sample window and smoothing stage.

Compressor and limiter parameters are published through atomic scalar values. Their processing snapshots those values without a mutex or blocking handoff. The EQ retains its bounded, non-blocking band handoff.

## Run locally

```sh
cmake --preset release
cmake --build --preset release --target wezt_realtime_dsp_test
ctest --test-dir build/release -R wezt_realtime_dsp_test --output-on-failure
```

The targeted Linux ThreadSanitizer build and test passed. The AddressSanitizer/UBSan build was attempted but did not complete within the available execution window, so it is not counted as passed. CI includes the stress test in its Release, Clang, ASan/UBSan and TSan test matrix, and in native Windows/macOS CTest runs.

## AddressSanitizer and UndefinedBehaviorSanitizer follow-up

The focused eight-insert stress target was built with GCC 14.2 using both
AddressSanitizer and UndefinedBehaviorSanitizer (`-fsanitize=address,undefined`)
and executed with leak detection and halt-on-error enabled. It completed all
6,000 callbacks with finite samples, zero render-thread heap allocations, and
concurrent parameter updates; no ASan, UBSan, or leak report was emitted.

The `asan` CMake preset uses a lower RelWithDebInfo optimization/debug-info
profile (`-O1 -g0`) to reduce sanitizer compile time on constrained machines.
This was a targeted DSP stress test, not a sanitizer run of every CTest target.

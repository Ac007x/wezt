# Repository recovery status

## Verified on 2026-11-26

The connected GitHub repository `Ac007x/wezt`, default branch `main`, was inspected through the GitHub API.

- The recursive Git tree contains 98 entries.
- It contains no `src/` paths.
- It contains no `.github/workflows/` paths.
- The Actions API reports zero workflow runs.
- The checked-in `CMakeLists.txt` references dozens of `src/` translation units, so the current checkout cannot build as a complete DAW source tree.

## Recovery source available

The canonical user-provided `wezt-source(1).zip` is available in the ChatGPT Library. It contains 296 archive entries, including 198 paths under `src/` and the original CI/release workflow definitions. The later SDK-gate preparation archive contains the source tree plus the VST3/CLAP integration gate hardening.

## Safe recovery policy

Do not replace the repository's root wholesale. Restore the missing source and workflow paths additively, preserving the existing root files and checking for path collisions. Then run the default Linux suite before enabling SDK-dependent VST3/CLAP jobs. Native Windows/macOS and real SDK plugin results must be reported separately from simulator tests.

## Current blocker

The repository is not yet restored by this change. This document records the verified gap so the build system cannot be described as complete until the missing source paths are committed and validated.

# Repository recovery status

## Verified on 2026-11-27

The connected GitHub repository `Ac007x/wezt`, default branch `main`, was inspected through the GitHub API.

- The recursive Git tree contains 98 entries.
- It contains no `src/` paths, even though the checked-in `CMakeLists.txt` references the application translation units.
- The default branch does not yet contain the canonical application source tree.
- The recovery PR remains a draft and must not be merged as a completed recovery.

## Canonical source and local validation

The canonical user-provided `wezt-source(1).zip` is present in the ChatGPT workspace. A cleaned restoration archive has been prepared from it:

- Archive: `wezt-source-cleaned.zip`
- SHA-256: `2cb8f563086c2d36a9568a8a8fc363799528b676a69cc88dc06a23464dbb90d6`
- 294 archive entries, including 176 source files under `src/` and 52 test files under `tests/`.
- AAX-specific scanner/twin code, build references, tests, and documentation were removed. The persisted plugin-format numeric identifiers were preserved by leaving the old numeric slot unused.
- Generic alternate-format matching remains available for scanned formats such as AU and VST3.

The cleaned source configured and built in Linux Release mode with VST3, CLAP, and desktop UI SDK integrations disabled. CTest passed **28/28 tests**, including the Windows and macOS backend simulators. These simulations are not native Windows/macOS builds. No real VST3 or CLAP SDK plugin build was performed in this environment.

## Safe recovery policy

Do not delete the current repository tree before the canonical source has been restored and compared. Restore the canonical files additively, review path collisions, and then remove obsolete files that are confirmed not to belong to the canonical tree. Run the default Linux suite before enabling SDK-dependent VST3/CLAP jobs. Report native Windows/macOS and real SDK plugin results separately from simulator tests.

## Current blocker

The cleaned source archive is prepared locally, but the available GitHub connection supports text-file writes and cannot transfer the complete local archive/directory. Therefore the missing source files have **not** yet been committed to GitHub, and no existing GitHub source files have been deleted. Keep this PR draft and unmerged until the full source tree is restored and verified on GitHub.

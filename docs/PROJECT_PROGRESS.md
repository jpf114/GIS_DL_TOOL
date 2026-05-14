# Project Progress

Last updated: 2026-05-14

## Summary

`GIS_DL_TOOL` is in an active productization phase. The repository already has substantial GIS/AI functionality, and the recent work has been focused on making the Windows GUI shell, runtime packaging, and verification flow stay aligned with the sibling tool `GIS_TOOL` outside business-specific functionality.

## What Is Verified Now

Verified on 2026-05-14 in this workspace:

- Release configure: `cmake --preset release`
- Release build: `cmake --build --preset release`
- Release install-tree regression: `ctest --test-dir build/release -C Release --output-on-failure -R release_install_tree_test`
- Release install: `cmake --install build/release --config Release`
- Installed CLI smoke: `install\bin\gis_ai_cli.exe help`
- Installed GUI smoke: `install\bin\gis-ai-gui.exe --self-test`
- Installed GUI startup survival check: launch `install\bin\gis-ai-gui.exe`, wait 3 seconds, confirm the process stays alive, then stop it intentionally
- Downstream `find_package` consumer verification: `powershell -ExecutionPolicy Bypass -File tests\verify_release_package_consumer.ps1 -RepoRoot <repo> -BuildDir <repo>\build\release`

## Recent Completed Work

- Main-window execution summary now resets correctly when history is cleared, the last task is deleted, or the user switches groups
- Background completion from another task group no longer overwrites the currently visible group's summary or status text
- Execution-state updates and function-panel state updates were consolidated into shared helpers to reduce UI drift
- GUI object names used by regression tests were de-duplicated where they previously pointed at multiple unrelated labels
- Startup/runtime initialization was aligned more closely with `GIS_TOOL` by resolving runtime data paths before Qt application construction
- Result-summary wording and selection-state prompts were reviewed side by side with `GIS_TOOL` and updated toward the same shell language
- Release install-tree verification is now an explicit CTest regression instead of a manual-only check
- Release package export now declares the transitive `nlohmann_json` dependency needed by downstream consumers
- A standalone downstream package-consumer verification script was added under `tests/verify_release_package_consumer.ps1`
- Release build, install tree, installed CLI, installed GUI self-test, installed GUI startup, and downstream consumer build/run were re-verified end to end
- GUI invalid-parameter fast-fail regression test (`gui_invalid_param_fast_fail_test`) added and passing
- Installed-tree business-action GUI regression test (`release_installed_gui_segment_test`) added and passing
- `gui_data_support` now provides 4 platform-level capabilities aligned with GIS_TOOL: result message localization (`localizeResultMessage`), action-specific parameter validation (`validateActionSpecificParams`), execute button state management (`buildExecuteButtonState`), and invalid param highlighting (`resolveHighlightedParamKey`)
- Task runner now localizes result messages before displaying them in the task center
- MainWindow now uses `buildExecuteButtonState` for unified button state logic and `validateActionSpecificParams` for secondary validation
- GUI shell file cross-tool review completed: 27 files enumerated and compared against GIS_TOOL, with structural differences documented
- `gui_data_support` fully ported with all platform-level capabilities: data detection (`detectDataKind`, `isSupportedDataPath`), data auto-fill (`inspectDataForAutoFill`, `shouldAutoFillLayerValue`, `shouldAutoFillExtentValue`), output path derivation (`buildSuggestedOutputPath`, `computeDerivedOutputUpdate`, `defaultSuffixForOutput`), file dialog config (`buildFileParamUiConfig`, `FileParamUiConfig`), param text (`findCommonParamText`, `actionDisplayName`, `ParamText`)
- `test_gui_queue` timing sensitivity fully resolved via `TaskRunner::setTaskStartDelayForTesting()` mock delay mechanism (8/8 consecutive runs passing)
- Cross-tool alignment check added as a CTest regression (`cross_tool_alignment_check`), enabled via `-DGIS_TOOL_SOURCE_DIR=...`
- All 13 CTest regressions now passing (100%)

## Current Strengths

- Clear segmentation-focused business workflow
- Shared/static library outputs, CLI, and Qt GUI all build from the same repo
- Task queue, persistence, rerun flow, and structured task results in the GUI
- Full `gui_data_support` platform-level capabilities integrated: data detection, auto-fill (CRS, extent, layer name), output path derivation, file dialog config, param text
- GUI result message localization and action-specific parameter validation
- Working Windows install layout with runnable installed binaries
- Downstream `find_package` consumption from the installed release tree now works in this workspace
- GUI regression coverage includes smoke, invalid-param fast-fail, and installed-tree business-action tests
- Cross-tool alignment check as a CTest regression
- `gis_ai_gui_lib` static library enables GUI unit testing (25 data support test cases)
- 14/14 CTest regressions passing (100%)

## Open Areas

- P2: Shared shell pieces to reduce independently drifting implementations
- Wider verification beyond the current Windows-focused checks

## Packaging Status

Current Windows release packaging has been validated for:

- `install/bin`
- `install/bin/platforms`
- `install/bin/sqldrivers`
- `install/share/proj`
- `install/share/gdal`
- `install/share/icons`

During install verification on 2026-05-14, the release install tree was re-checked after the latest GUI state-sync fixes. Runtime dependency scanning succeeded, the installed CLI remained runnable, the installed GUI self-test returned successfully, and a normal GUI process stayed alive for at least 3 seconds before being intentionally stopped.

Downstream package verification on 2026-05-14 also confirmed that a separate consumer CMake project can `find_package(gis_ai CONFIG REQUIRED)`, link both shared and static imported targets, build successfully, and run against the installed release tree when launched through the standalone verification script.

## Honest Boundaries

- This document does not claim full cross-platform readiness.
- This document does not claim the entire current release CTest sweep is green; an unrelated timing-sensitive `test_gui_queue` failure was still observed while doing packaging follow-up work.
- This document does not treat cross-tool alignment as finished.

## Next Practical Moves

1. P2: Extract shared shell pieces to reduce independently drifting implementations
2. Keep updating repo docs so they match the newest verified state

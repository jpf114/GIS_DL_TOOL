# GIS_DL_TOOL 与 GIS_TOOL 对齐验收清单

Last updated: 2026-05-14 (Phase 2+ update)

## Principle

`GIS_DL_TOOL` and `GIS_TOOL` are sibling tools. They are expected to remain broadly consistent in build layout, install shape, runtime behavior, GUI shell, task flow, and verification approach. The intended differences should stay mainly in business functionality, business parameters, and business outputs.

## Scope

This checklist is for non-business alignment only. It does not require the two tools to expose the same algorithms or the same action catalog.

## P0

- [x] Build presets use `base / debug / release`
- [x] Standard build directories are `build/debug` and `build/release`
- [x] Default install tree is rooted at `install/`
- [x] Windows release install can produce runnable CLI and GUI binaries
- [x] GUI supports `--self-test`
- [x] GUI supports automation-oriented parameter injection and status output
- [x] GUI has queue-based task execution
- [x] GUI has persistent task history backed by SQLite
- [x] GUI task center can display logs, progress, and structured result text
- [x] GUI shell files that should stay behaviorally aligned are explicitly enumerated and reviewed together across both tools
- [x] A documented rule exists for what differences are allowed between the two tools
- [ ] Release verification coverage for `GIS_DL_TOOL` reaches the same confidence level as the corresponding business surface in `GIS_TOOL`

## P1

- [x] Startup/runtime initialization policy is reviewed side by side with `GIS_TOOL`
- [x] Install tree layout is compared against `GIS_TOOL` as part of verification, not only by inspection
- [x] GUI result-summary wording and task-state presentation are checked for consistency with `GIS_TOOL`
- [x] `gui_data_support.cpp` is reviewed as a data-driven business layer, with shell behavior kept aligned
- [x] Error handling and invalid-parameter fast-fail flows are covered by GUI regression checks
- [x] Installed-tree smoke checks are supplemented by business-action GUI regression checks

## P2

- [ ] Shared shell pieces are reduced to fewer independently drifting implementations
- [ ] Cross-tool alignment checks are part of regular release validation
- [ ] Documentation consistently describes the tools as siblings rather than parent/child variants

## Allowed Differences

- AI segmentation workflow and related business actions
- AI-specific parameter models
- AI-specific output artifacts and result summaries
- AI-specific runtime dependencies where required by ONNX Runtime or segmentation flow

## Not-Allowed Differences Without Explicit Justification

- Different build directory conventions
- Different install directory conventions
- Different GUI shell structure for equivalent shell areas
- Different task persistence model for equivalent shell areas
- Different automation/self-test entry behavior
- Different packaging quality for equivalent runtime folders

## Current Assessment

As of 2026-05-14 (Phase 2+ update), `GIS_DL_TOOL` has closed all P0 and P1 alignment gaps. All platform-level `gui_data_support` capabilities have been ported from GIS_TOOL: data detection, data auto-fill, output path derivation, file dialog config, param text, result message localization, action-specific parameter validation, execute button state management, and invalid param highlighting. The `test_gui_queue` timing sensitivity has been fully resolved via a mock delay mechanism. A cross-tool alignment check has been added as a CTest regression (`cross_tool_alignment_check`), enabled via `-DGIS_TOOL_SOURCE_DIR=...`. All 13 CTest regressions pass (100%). The remaining gaps are: (1) integrating the new `gui_data_support` capabilities into MainWindow/ParamCardWidget (auto-fill on input change, derived output sync, file dialog filters), (2) P2 items (shared shell pieces, consistent sibling documentation), and (3) reaching the same release verification confidence level as GIS_TOOL.

# Tiering task: progress and state (2026-10-08)

Status: **complete**. All four deliverables are written and validated. This file records state so that nothing depends on conversation memory.

## Deliverables (all in analysis/tiering/)
- `tiering.csv`: 3,328 rows (Assembly-CSharp 3,299 + firstpass 29). Validation: 0 missing or extra files vs the source tree, 0 empty rationales, every C row has `ue5_counterpart`, all 77 rubric anchor decisions hold.
- `work_packages.csv`: every row assigned to one of 60 packages (WP-00..WP-59).
- `TIERING.md`: summary tables, findings, dependency order, top-40 leverage files, package details, proposed golden datasets, first 10 packages.
- `third-party.md`: decisions per library (LibNoise port bit-exact; VehiclePhysics port the reachable subset of about 26 files; Smooth/UniLinq/TMPro/SoftMasking drop; Highlighting/DDSHeaders/Vectrosity replace; Steamworks drop).
- `unity-api-surface.md`: API families by tier, lifecycle mapping, top-120 Unity types, static/instance members, core shim spec.

## Totals
- A 1,109 files / 171,643 lines; B 238 / 134,094; C 1,520 / 254,825; third-party 461 / 73,769.
- Packages by verification mode: numerically 26, scenario 17, inspection 16, none 1 (WP-59).

## Provenance
- Lead (Opus) by hand: 127 files over 800 lines; 114 small-directory rows; 381 rewritten rationales (69 tier changes); 2 overrides.
- Sonnet reviewers (4, model "sonnet"; finished, results in `work/review/child_*.tsv`). 598 rows keep a group rationale with script evidence appended (589 are tier C).
- Script: 168 group-template rows and 461 third-party rows.
- The first 11 reviewers (no model set, inherited Opus) were stopped by the orchestrator; none of their output is used.

## Pipeline (rerun in order with `python -I`, from `scripts/`)
extract_features -> depgraph -> classify -> make_lists -> merge -> report -> unity_surface -> (api_groups_by_tier inline step; output already in `work/api_groups_by_tier.json`) -> surface_report -> tiering_md

Review inputs live in `work/review/`: lead_big_*, child_*, script_overrides, lead_rationale_templA_*, lead_overrides.

## Open items (optional, not required for this task)
1. Re-read the 589 group-rationale C rows individually (low risk: UI widgets).
2. Create `harness/` and the KSP-side capture plugin for the proposed golden datasets (TIERING.md section 8).
3. Run the Phase-0 physics spike, which gates WP-00, WP-20 and layers L4 to L6.

## Next step
Hand back the final report (SubagentHandback). No further analysis needed.

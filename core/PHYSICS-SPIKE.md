# Physics spike (Jolt): what WP-19 / WP-20 / WP-12 should build

Status: 2026-10-11, branch `jolt-spike` (from main `af3798c6`). Spike code: `core/spikes/jolt/` (opt-in CMake target, never part of
`run_tests.ps1` / CI). Raw results: `core/spikes/jolt/results/*.md|csv`. Progress log: `core/PROGRESS-jolt.md`.
Inputs: [PHYSICS-ENGINE-REVIEW.md](../analysis/physics/PHYSICS-ENGINE-REVIEW.md), [PROFILE.md](../analysis/physics/PROFILE.md),
[TIERING.md](../analysis/tiering/TIERING.md). C# citations are relative to `decompiled/KSP-Decompiled/Assembly-CSharp/`.

__TLDR__

## Contents

1. [What the spike is](#1-what-the-spike-is)
2. [Measurements](#2-measurements)
3. [Recommendation for WP-20](#3-recommendation-for-wp-20)
4. [Exact / approximate / changed, per optimisation](#4-exact--approximate--changed-per-optimisation)
5. [The interface WP-19 / WP-20 / WP-12 need from kspcore](#5-the-interface-wp-19--wp-20--wp-12-need-from-kspcore)
6. [Risks](#6-risks)
7. [Package split and gates](#7-package-split-and-gates)
8. [Reproducing](#8-reproducing)

__BODY__

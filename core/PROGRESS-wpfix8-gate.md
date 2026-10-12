# PROGRESS wpfix8 gate
- Merged origin/main ea132145 (wp19a) into wpfix8 (019785a5): ~Vessel took main's wp19a form (prunes parts/rootPart to Part::allParts); wpfix8's extra std::find guard dropped as redundant; STATUS row renumbered 43.
- Windows -Fresh (CMAKE_BUILD_PARALLEL_LEVEL=6): ctest 35/35, unit 806 / 1,326,291, 28 compare reports all max_abs 0 / max_ulp 0.
- WSL g++ ASan+UBSan+LSan whole binary on the merged tree (~/wpfix8gate): 806 / 1,326,275, 0 reports.
- Standalone check of the unsigned-wrap rewrites (200,000 random cases) vs Python mod-2^64/2^32 arithmetic: 0 mismatches, UBSan clean.
- STATUS/README sanitizer numbers updated to 806 / 1,326,275 (Windows 1,326,291), 35/35.

# PROGRESS wp20a gate
- [x] 1 fresh run (head 73ec7f2e tree == ce9dbef0 + .gitignore + Build/CMakeLists.txt tracked): 34/34, unit 718 / 1,094,123, WP-20a 14 / 147; 27 compare reports ok, 0 fails, overlap with main build identical
- [x] 2 vendoring byte-identical to v5.6.0 archive (sha256 6e069ee0...), 561 files, LICENSE; no FetchContent/ExternalProject/http in CMake
- [x] 3 tests meaningful; sabotage (same-vessel filter flipped) fails 3 assertions; restored
- [x] 4 cites 15 / 0 mismatches; Krakensbane.cs port read line by line
- [x] 5 spike numbers match results files
- [ ] 6 CI for 73ec7f2e (ce9dbef0 runs FAILED: Build/ untracked by .gitignore; fixed in 73ec7f2e)
- [x] 7 merge-tree clean

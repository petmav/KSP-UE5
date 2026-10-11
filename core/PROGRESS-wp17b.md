# WP-17b progress
- [x] tools/wp17/mu_transforms.py + core/data/engine_thrust_transforms.json (19 parts incl. sepMotor1)
- [x] tools/support/engine_thrust_transforms.hpp (+ EditorHierarchy, fully right-nested World(part, &chain)), UnityNative::Multiply (w = (lw*rw - lx*rx) - (ly*ry + lz*rz)) / Inverse / Normalize
- [x] tolerance rules + exclusions file deleted
- [x] merged main 13750b33 (only STATUS.md conflicted: unit row)
- [x] gate: 15 of 17 crafts 0 ULP; RED on Dynawing (14 rows, 4 ULP) and Stearwing A300 (6 rows, 2 ULP); no exclusions by brief (Dynawing not on the allowed list)
- [x] Windows fresh: 33/34 (deltav_stages red), unit 569 cases / 935,432 assertions
- [ ] WSL run (/tmp/wp17b_clang.log), commit

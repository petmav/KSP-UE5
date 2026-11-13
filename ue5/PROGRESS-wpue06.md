# PROGRESS WP-UE-06 (asset extraction tool)

- [x] read role, memory, DESIGN, WP-12c tools; survey of game assets
- [x] tool: ue5/Tools/extract_game_assets.py + ksp_unity / ksp_textures / ksp_mesh / ksc_scenery
- [x] full run: 2910 files, 1127.1 MB, 7 missing; manifest sha256 f9369b57...; two runs from scratch: whole output tree identical (2911 files)
- [x] verify_game_assets.py: 0 mismatches; mutation test (6 edits) -> 7 mismatches reported
- [x] README.md, STATUS / README sections, progress.csv WP-UE-06 = 1.0, memory note
- [ ] final verify with --summary, pr_review after commit, merge origin/main, push, PR
Env: python = C:/Projects/KSP-UE5/harness/tools-cache/venv/Scripts/python.exe -I ; game = C:/Projects/KSP-UE5/decompiled/KSP-dev

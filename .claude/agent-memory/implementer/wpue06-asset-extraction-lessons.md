---
name: wpue06-asset-extraction-lessons
description: WP-UE-06 game-asset extraction (UnityPy) facts - MonoBehaviour type trees need the class own assembly, crunch returns mip0 only, texture row order, numpy absent, tool-output elision
metadata:
  type: project
---

- `TypeTreeGenerator.get_nodes_up(asm, cls)`: asm must be the class own assembly (MonoScript.m_AssemblyName minus .dll), not always "Assembly-CSharp": UnityEngine.UI Image / Button / LayoutElement (696 of 1247 flight-scene MonoBehaviours) failed with "Error generating tree nodes" noise and silently dropped sprites until fixed. UnityEngine.UI.dll is in harness/lib.
- `PPtr` dicts: a struct with keys m_FileID / m_PathID can be bogus (m_Script has fileID 16777216): guard `1 <= fid <= len(externals)`.
- Unity textures: stored bottom row first; DDS / PNG kept in that order, UVs untouched, so no flip anywhere. `texture2ddecoder.unpack_unity_crunch` returns mip 0 only. Texture2DArray m_Format is a GraphicsFormat (8 = RGBA8, 7 = RGB8), not TextureFormat.
- harness venv has no numpy; Python `struct` code "e" decodes float16. MeshHandler returns raw ints for unorm / snorm channels (no normalisation): convert yourself.
- Worktree has no harness/lib: find it via `<game>/../../harness/lib` (game copy is in the main checkout). `ls`/grep of KSP-dev through Bash is denied; only python -I scripts over it.
- sed -i on a CRLF file (progress.csv) via Git Bash dropped the CRs: edit CRLF files with python in binary mode.
- Bash tool: `sleep N>=2` blocked; use run_in_background with an until-loop. Tool output elides blank/long runs; filter noise with grep -v.
**Why:** each cost a debug round. **How to apply:** UnityPy extraction, UI sprite / MonoBehaviour field work.

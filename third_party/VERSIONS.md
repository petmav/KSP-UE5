# Vendored third-party code

Pinned single-header releases, copied verbatim. Downloaded 2026-10-09.

| library | version | file | source URL | sha256 |
|---|---|---|---|---|
| doctest | 2.4.12 | `doctest/doctest.h` | https://raw.githubusercontent.com/doctest/doctest/v2.4.12/doctest/doctest.h | `94029a7d32da24a56249658147dbd2b33ff0b9ed665295cbbaf19aafff5b0ced` |
| nlohmann/json | 3.12.0 | `nlohmann/json.hpp` | https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp | `aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63` |

Use: doctest for `tests/` only; nlohmann/json for the golden runner (`tools/golden_runner/`) only. `kspcore` itself has
no third-party dependency.

Verify: `sha256sum third_party/doctest/doctest.h third_party/nlohmann/json.hpp` (Git Bash) or
`Get-FileHash -Algorithm SHA256 <file>` (PowerShell).

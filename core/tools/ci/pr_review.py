#!/usr/bin/env python3
"""Scripted review of a branch against its base: the checks a reviewer used to run by hand.

Usage: python -I core/tools/ci/pr_review.py [--base REF]   (default base: origin/main; exit 1 on a failure)

Checks, on the files and lines the branch changes (git diff BASE...HEAD):
  1. harness/compare/tolerances.json is unchanged (golden tolerances are never loosened).
  2. A change to core/tests/tolerances_capture.json comes with a core/STATUS.md change (each relaxation is documented).
  3. No added line in core/src or core/include/ksp calls std:: maths (std::sin, std::pow ...): ported code goes through
     ksp::dotnet. The libm emulation itself (dotnet/ dirs) is exempt; a deliberate native call carries `// std-math-ok`.
  4. The C# citations in every changed core/src and core/include/ksp source pass core/tools/wp28/check_cites.py.
  5. (warning only) Ported code changed but analysis/tiering/progress.csv did not.
Runs in CI (.github/workflows/ci.yml, job `review`) and locally before a PR.
"""
import argparse
import os
import re
import subprocess
import sys

# The checkout being reviewed is the one the command runs in (a worktree may hold another branch).
ROOT = subprocess.run(["git", "rev-parse", "--show-toplevel"], check=True, capture_output=True, text=True).stdout.strip()
CITES = os.path.join(ROOT, "core", "tools", "wp28", "check_cites.py")
STD_MATH = re.compile(r"\bstd::(sin|cos|tan|exp|exp2|log|log2|log10|pow|sqrt|cbrt|acos|asin|atan|atan2|sinh|cosh|tanh|hypot|fmod)\s*\(")
SOURCE = re.compile(r"^core/(src|include/ksp)/.*\.(cpp|hpp|h|inl)$")
GITHUB = os.environ.get("GITHUB_ACTIONS") == "true"


def git(*args):
    return subprocess.run(["git", "-C", ROOT, *args], check=True, capture_output=True, text=True).stdout


def report(kind, msg, path=None, line=None):
    if GITHUB:
        loc = f" file={path}" + (f",line={line}" if line else "") if path else ""
        print(f"::{kind}{loc}::{msg}")
    else:
        where = f"{path}:{line}: " if line else (f"{path}: " if path else "")
        print(f"{kind.upper()}: {where}{msg}")


def added_lines(base, path):
    """(line number, text) of every line the branch adds to path."""
    out, n = [], 0
    for raw in git("diff", "-U0", f"{base}...HEAD", "--", path).splitlines():
        if raw.startswith("@@"):
            n = int(re.match(r"@@ -\S+ \+(\d+)", raw).group(1))
        elif raw.startswith("+") and not raw.startswith("+++"):
            out.append((n, raw[1:]))
            n += 1
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", default="origin/main")
    base = ap.parse_args().base
    changed = [p for p in git("diff", "--name-only", "--diff-filter=AMR", f"{base}...HEAD").splitlines() if p]
    failures = 0

    if "harness/compare/tolerances.json" in changed:
        report("error", "golden tolerances changed; they are never loosened", "harness/compare/tolerances.json")
        failures += 1

    if "core/tests/tolerances_capture.json" in changed and "core/STATUS.md" not in changed:
        report("error", "capture tolerance changed without a core/STATUS.md entry", "core/tests/tolerances_capture.json")
        failures += 1

    sources = [p for p in changed if SOURCE.match(p) and "/dotnet/" not in p]
    for path in sources:
        for n, text in added_lines(base, path):
            code = text.split("//", 1)[0]
            if STD_MATH.search(code) and "std-math-ok" not in text:
                report("error", "std:: maths in ported code; use ksp::dotnet (or mark `// std-math-ok`)", path, n)
                failures += 1

    if sources:
        r = subprocess.run([sys.executable, "-I", CITES, *[os.path.join(ROOT, p) for p in sources]],
                           capture_output=True, text=True)
        if r.returncode:
            for line in r.stdout.splitlines():
                if line.strip():
                    report("error", "C# cite: " + line.strip())
            failures += 1

    if sources and "analysis/tiering/progress.csv" not in changed:
        report("warning", "ported code changed but analysis/tiering/progress.csv did not")

    print(f"pr_review: {len(changed)} changed files, {len(sources)} sources checked, {failures} failing checks")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())

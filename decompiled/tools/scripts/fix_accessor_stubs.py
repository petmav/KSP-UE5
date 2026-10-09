"""Guide 3.3: for every CS0683 ("explicit method implementation cannot implement ... because it is an accessor")
delete the offending method including the [SpecialName] attribute above it.

Usage: fix_accessor_stubs.py <errors.log> <project dir>
Prints one markdown table row per removed stub (for FIXES.md).
"""
import re, sys, collections, os

log, proj = sys.argv[1], sys.argv[2]
pat = re.compile(r"^(?P<file>.+?)\((?P<line>\d+),(?P<col>\d+)\): error CS0683: (?P<msg>.*?) \[")
hits = collections.defaultdict(dict)
for raw in open(log, encoding="utf-8-sig", errors="replace"):
    m = pat.match(raw.strip())
    if m:
        hits[m["file"]][int(m["line"])] = m["msg"]

rows = []
for path, lines_map in sorted(hits.items()):
    with open(path, encoding="utf-8-sig", newline="") as fh:
        text = fh.read()
    nl = "\r\n" if "\r\n" in text else "\n"
    had_bom = open(path, "rb").read(3) == b"\xef\xbb\xbf"
    lines = text.split(nl)
    for ln in sorted(lines_map, reverse=True):
        i = ln - 1                        # signature line index
        assert lines[i - 1].strip() == "[SpecialName]", (path, ln, lines[i - 1])
        assert lines[i + 1].strip() == "{", (path, ln)
        indent = lines[i + 1][: len(lines[i + 1]) - len(lines[i + 1].lstrip())]
        j = i + 2
        while lines[j] != indent + "}":
            j += 1
        body = [l.strip() for l in lines[i + 2 : j]]
        assert len(body) == 1 and re.fullmatch(r"(return base\.\w+;|base\.\w+ = value;)", body[0]), (path, ln, body)
        sig = lines[i].strip()
        start, end = i - 1, j            # inclusive range to delete
        if start > 0 and lines[start - 1].strip() == "":
            start -= 1                   # drop the blank separator line before it
        elif lines[end + 1].strip() == "":
            end += 1
        del lines[start : end + 1]
        rel = os.path.relpath(path, proj).replace("\\", "/")
        rows.append(f"| {rel} | {ln} | CS0683: {lines_map[ln]} | Deleted `[SpecialName] {sig} {{ {body[0]} }}` (compiler regenerates this bridge stub automatically) |")
    out = nl.join(lines)
    with open(path, "w", encoding="utf-8-sig" if had_bom else "utf-8", newline="") as fh:
        fh.write(out)

for r in sorted(rows):
    print(r)
print(f"<!-- removed {len(rows)} stubs in {len(hits)} files -->")

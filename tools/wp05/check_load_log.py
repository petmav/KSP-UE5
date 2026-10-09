"""Compare kspcore's GameDatabase load order with the game's own KSP.log of the C9 capture run.

    python -I check_load_log.py <candidate gamedatabase_load.log> <KSP.log>

KSP logs one line per loaded asset and config (GameDatabase.cs:1247/1284/1330 "Load(Audio|Texture|Model): <url>",
GameDatabase.cs:952 "Config(<type>) <url>"). The candidate log (written by golden_runner part_database) must contain
exactly the same lines in the same order. Exit 0 when identical, 1 on any difference, 0 with a note when KSP.log is
absent (the log is a local artefact of the capture run, decompiled/logs/).
"""
import os
import re
import sys

PAT = re.compile(r"^\[LOG [0-9:.]+\] ((?:Load\((?:Audio|Texture|Model)\):|Config\([^)]*\)) .*)$")


def main(argv):
    if len(argv) != 2:
        print(__doc__)
        return 2
    cand_path, log_path = argv
    if not os.path.exists(log_path):
        print("check_load_log: %s not found; skipped" % log_path)
        return 0
    with open(log_path, encoding="utf-8", errors="replace") as f:
        game = [m.group(1).rstrip() for m in (PAT.match(l.rstrip("\r\n")) for l in f) if m]
    with open(cand_path, encoding="utf-8") as f:
        cand = [l.rstrip("\r\n").rstrip() for l in f if l.strip()]
    n = min(len(game), len(cand))
    for i in range(n):
        if game[i] != cand[i]:
            print("check_load_log: line %d differs:\n  game:      %s\n  candidate: %s" % (i + 1, game[i], cand[i]))
            return 1
    if len(game) != len(cand):
        print("check_load_log: %d lines in the game log, %d in the candidate" % (len(game), len(cand)))
        return 1
    kinds = {}
    for l in game:
        k = l.split(")")[0] + ")"
        kinds[k] = kinds.get(k, 0) + 1
    print("check_load_log: %d lines identical (%s)" % (len(game), ", ".join(
        "%s %d" % (k, v) for k, v in sorted(kinds.items(), key=lambda kv: -kv[1])[:6])))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

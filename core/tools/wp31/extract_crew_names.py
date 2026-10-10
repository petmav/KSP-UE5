"""Extract the CrewGenerator name tables (Unity TextAssets) from a KSP install (WP-31, local only).

CrewGenerator.cs loads  Resources.Load("TextAssets/CrewGenerator/<lang>/KerbalNamesMale" | "KerbalNamesFemale" |
"KerbalSpecialNamesMale" | "KerbalSpecialNamesFemale")  -- ConfigNode text with a node of the same name holding `Element = <name>`
lines. They live in KSP_x64_Data/resources.assets (or a sharedassets file) and are not in the GameData mirror.

Usage:  python -I extract_crew_names.py <KSP_x64_Data dir> <out dir>
Writes <out dir>/<lang>/<AssetName>.txt (verbatim text) and <out dir>/crew_names.json {lang: {asset: [names...]}}.
Needs UnityPy (the venv of harness/tools/extract_assets.py). NOT RUN in the WP-31 session (no access to the game install there):
adjust the container match below if the assets are not found.
"""
import json
import os
import sys

import UnityPy

WANTED = {"KerbalNamesMale", "KerbalNamesFemale", "KerbalSpecialNamesMale", "KerbalSpecialNamesFemale"}


def main(data_dir, out_dir):
    env = UnityPy.load(data_dir)
    result = {}
    for path, obj in env.container.items():
        if "crewgenerator" not in path.lower():
            continue
        data = obj.read()
        if obj.type.name != "TextAsset" or data.m_Name not in WANTED:
            continue
        parts = path.replace("\\", "/").split("/")
        low = [x.lower() for x in parts]  # container paths may be lower-cased: match case-insensitively, keep the language folder as is
        lang = parts[low.index("crewgenerator") + 1] if "crewgenerator" in low else "en-us"
        text = data.m_Script if isinstance(data.m_Script, str) else bytes(data.m_Script).decode("utf-8-sig")
        os.makedirs(os.path.join(out_dir, lang), exist_ok=True)
        with open(os.path.join(out_dir, lang, data.m_Name + ".txt"), "w", encoding="utf-8", newline="") as f:
            f.write(text)
        names = [ln.split("=", 1)[1].strip() for ln in text.splitlines() if ln.strip().startswith("Element")]
        result.setdefault(lang, {})[data.m_Name] = names
    with open(os.path.join(out_dir, "crew_names.json"), "w", encoding="utf-8") as f:
        json.dump(result, f, indent=1, ensure_ascii=False)
    print({lang: {k: len(v) for k, v in d.items()} for lang, d in result.items()})


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])

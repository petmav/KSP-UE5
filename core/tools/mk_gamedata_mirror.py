"""Builds core/testdata/GameData-mirror/ from a KSP 1.12.5 install (WP-00L).

    python -I tools/mk_gamedata_mirror.py --src C:/Projects/KSP-UE5/decompiled/KSP-dev --dst testdata/GameData-mirror
    python -I tools/mk_gamedata_mirror.py --dst testdata/GameData-mirror --verify      (no game install needed)

The mirror is a game root (what ksp::GameData::Load takes as gameRoot) that keeps everything the tests read from the real
install and nothing else:
  * every *.cfg under GameData/ byte for byte (including Squad/Localization/dictionary.cfg), plus Physics.cfg and
    PartDatabase.cfg at the root,
  * for every other file under GameData/ (textures, models, audio, .ksp assemblies ...) a ZERO-BYTE placeholder with the
    exact relative path (EXCEPT .dds/.mbm: the loader reads their headers, so those keep the original header bytes only), so UrlDir sees the same directory tree, file names, extensions and load order,
  * zero-byte placeholders for the Unity asset files (KSP_x64_Data/*.assets, *.resS) that the asset indexes under
    core/data/ cite (tests only check that they exist; the extracted data itself is committed under core/data/),
  * MANIFEST.json: counts by extension, SHA-256 of every cfg, the list of placeholder-only paths' count, source build id.
Original game data stays private: the repository is private and nothing derived from it is ever distributed.
Tests default to this mirror; set env KSP_GAMEDATA_ROOT to a real install to run them against it.
"""
import argparse
import collections
import hashlib
import json
import os
import shutil
import sys


def sha256(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


def texture_header(path, ext):
    """GameDatabase reads only the headers of .dds (128 bytes: magic, size, pixel format, caps) and .mbm (7-bit-length-prefixed
    name + 16 bytes) to decide whether a texture loads and whether it is a normal map (game_database.cpp ReadDdsHeader /
    ReadMbmHeader). A zero-byte placeholder would make every such texture 'unresolved', so these two types keep exactly
    that header and nothing else; every other non-cfg file is an empty placeholder."""
    if ext == '.dds':
        with open(path, 'rb') as f:
            return f.read(128)
    if ext == '.mbm':
        with open(path, 'rb') as f:
            d = f.read(4096)
        n = shift = i = 0
        while True:
            c = d[i]
            i += 1
            n |= (c & 0x7F) << shift
            if not c & 0x80:
                break
            shift += 7
        return d[:i + n + 16]
    return b''


def collect_asset_files(core_data):
    files = set()

    def walk(o):
        if isinstance(o, dict):
            for v in o.values():
                walk(v)
        elif isinstance(o, list):
            for v in o:
                walk(v)
        elif isinstance(o, str) and o.startswith('KSP_x64_Data/'):
            files.add(o)

    for p in [os.path.join(core_data, 'kerbol_body_assets.json')] + sorted(
            os.path.join(core_data, 'terrain', n) for n in os.listdir(os.path.join(core_data, 'terrain')) if n.endswith('.json')):
        with open(p, encoding='utf-8') as f:
            walk(json.load(f))
    return sorted(files)


def build(src, dst, core_data):
    gd_src = os.path.join(src, 'GameData')
    if not os.path.isdir(gd_src):
        sys.exit('no GameData under ' + src)
    if os.path.isdir(dst):
        shutil.rmtree(dst)
    os.makedirs(dst)
    cfg = {}
    ext_counts = collections.Counter()
    placeholders = 0
    header_stubs = collections.Counter()
    for root, dirs, files in os.walk(gd_src):
        dirs.sort()
        rel_dir = os.path.relpath(root, src).replace(os.sep, '/')
        os.makedirs(os.path.join(dst, rel_dir), exist_ok=True)
        for name in sorted(files):
            rel = rel_dir + '/' + name
            ext = os.path.splitext(name)[1].lower() or '(none)'
            ext_counts[ext] += 1
            out = os.path.join(dst, rel)
            if name.endswith('.cfg'):
                shutil.copyfile(os.path.join(root, name), out)
                cfg[rel] = sha256(out)
            else:
                head = texture_header(os.path.join(root, name), ext)
                with open(out, 'wb') as f:
                    f.write(head)
                placeholders += 1
                if head:
                    header_stubs[ext] += 1
    for name in ('Physics.cfg', 'PartDatabase.cfg'):
        shutil.copyfile(os.path.join(src, name), os.path.join(dst, name))
        cfg[name] = sha256(os.path.join(dst, name))
    assets = collect_asset_files(core_data)
    for rel in assets:
        os.makedirs(os.path.join(dst, os.path.dirname(rel)), exist_ok=True)
        open(os.path.join(dst, rel), 'wb').close()
    build_id = ''
    bp = os.path.join(src, 'buildID64.txt')
    if os.path.isfile(bp):
        build_id = open(bp, encoding='utf-8', errors='replace').read().strip()
    manifest = {
        'schema': 'ksp-gamedata-mirror/1',
        'source': 'KSP 1.12.5 install (decompiled/KSP-dev); buildID64.txt: ' + build_id,
        'counts': {
            'gamedata_files': sum(ext_counts.values()),
            'cfg_verbatim': sum(1 for k in cfg if k.startswith('GameData/')),
            'root_cfg_verbatim': sum(1 for k in cfg if not k.startswith('GameData/')),
            'placeholders_in_gamedata': placeholders,
            'header_only_stubs_in_gamedata': dict(sorted(header_stubs.items())),   # .dds 128 bytes, .mbm header; the rest are 0 bytes
            'zero_byte_unity_asset_placeholders': len(assets),
            'by_extension_in_gamedata': dict(sorted(ext_counts.items(), key=lambda kv: (-kv[1], kv[0]))),
        },
        'unity_asset_placeholders': assets,
        'cfg_sha256': dict(sorted(cfg.items())),
    }
    with open(os.path.join(dst, 'MANIFEST.json'), 'w', encoding='utf-8', newline='\n') as f:
        json.dump(manifest, f, indent=1, sort_keys=False)
        f.write('\n')
    return manifest


def verify(dst):
    with open(os.path.join(dst, 'MANIFEST.json'), encoding='utf-8') as f:
        m = json.load(f)
    bad = 0
    for rel, h in m['cfg_sha256'].items():
        p = os.path.join(dst, rel)
        if not os.path.isfile(p) or sha256(p) != h:
            print('MISMATCH', rel)
            bad += 1
    n = 0
    for root, _, files in os.walk(os.path.join(dst, 'GameData')):
        n += len(files)
    if n != m['counts']['gamedata_files']:
        print('file count', n, '!=', m['counts']['gamedata_files'])
        bad += 1
    print('verify:', 'FAILED' if bad else 'ok', '(%d cfg, %d GameData files)' % (len(m['cfg_sha256']), n))
    return bad


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--src')
    ap.add_argument('--dst', required=True)
    ap.add_argument('--core-data', default=os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'data'))
    ap.add_argument('--verify', action='store_true')
    a = ap.parse_args()
    if a.verify:
        sys.exit(1 if verify(a.dst) else 0)
    if not a.src:
        sys.exit('--src required')
    m = build(a.src, a.dst, a.core_data)
    print(json.dumps(m['counts'], indent=1))
    sys.exit(1 if verify(a.dst) else 0)


if __name__ == '__main__':
    main()

"""Shared loader for the WP-UE-06 extraction tool: the game's Unity-serialized assets through UnityPy.

Read-only on the game copy. Every object is addressed by a key (asset file basename, pathID), the provenance record the manifest
stores. MonoBehaviour type trees come from the game's own assemblies (harness/lib, the same method as
core/tools/mucollider/extract_scenery_colliders.py and core/tools/extract_body_assets.py) because the player build ships none.
"""
import hashlib
import os

import UnityPy
import UnityPy.helpers.TypeTreeHelper as TH
from UnityPy.helpers.TypeTreeGenerator import TypeTreeGenerator
from UnityPy.streams import EndianBinaryReader

UNITY_VERSION = "2019.4.18f1"
BCL = ["mscorlib.dll", "System.dll", "System.Core.dll", "System.Xml.dll", "Mono.Security.dll",
       "System.Configuration.dll", "System.Security.dll"]
BUILTIN_TAGS = {0: "Untagged", 1: "Respawn", 2: "Finish", 4: "EditorOnly", 5: "MainCamera", 6: "Player", 7: "GameController"}


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 22), b""):
            h.update(chunk)
    return h.hexdigest()


def sha256_bytes(b):
    return hashlib.sha256(b).hexdigest()


def find_lib_dir(game_root, explicit=None):
    """harness/lib holds the de4dot-original assemblies the type-tree generator needs. A worktree has no copy (gitignored), the
    main checkout does: <game_root>/../../harness/lib is the main checkout when the game copy is decompiled/KSP-dev."""
    cands = [explicit] if explicit else []
    cands += [os.path.join(game_root, "..", "..", "harness", "lib"),
              os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "harness", "lib")]
    for c in cands:
        if c and os.path.isfile(os.path.join(c, "Assembly-CSharp.de4dot-original.dll")):
            return os.path.abspath(c)
    raise SystemExit("harness/lib not found (pass --lib); tried: %s" % cands)


class Game:
    def __init__(self, game_root, lib_dir=None):
        self.game_root = os.path.abspath(game_root)
        self.data_dir = os.path.join(self.game_root, "KSP_x64_Data")
        lib_dir = find_lib_dir(self.game_root, lib_dir)
        gen = TypeTreeGenerator(UNITY_VERSION)
        for f in sorted(os.listdir(lib_dir)):
            if f.endswith(".dll") and f != "Assembly-CSharp.dll":
                gen.load_dll(open(os.path.join(lib_dir, f), "rb").read())
        for f in BCL:
            gen.load_dll(open(os.path.join(self.data_dir, "Managed", f), "rb").read())
        self.gen = gen
        self.env = UnityPy.load(self.data_dir)
        self.env.typetree_generator = gen
        self.objs = {}
        for o in self.env.objects:
            self.objs[(o.assets_file.name, o.path_id)] = o
        self.files = {os.path.basename(n): f for n, f in self.env.files.items()}
        self._tt = {}
        self._mb_nodes = {}
        self._cls = {}
        self._asm = {}
        self.tags = list(self.tt(self.first("TagManager"))["tags"])

    # ---- lookup -------------------------------------------------------------------------------------------------------------
    def first(self, type_name, name=None, file=None):
        hits = self.all(type_name, name, file)
        if len(hits) != 1:
            raise RuntimeError("%s %r in %r: %d hits" % (type_name, name, file, len(hits)))
        return hits[0]

    def all(self, type_name, name=None, file=None):
        out = []
        for k, o in self.objs.items():
            if o.type.name != type_name or (file and k[0] != file):
                continue
            if name is not None and self.name_of(k) != name:
                continue
            out.append(k)
        return sorted(out)

    def type_of(self, key):
        return self.objs[key].type.name

    def tt(self, key):
        """The type tree as a dict (cached). MonoBehaviour classes read through the generated trees."""
        if key not in self._tt:
            self._tt[key] = self.objs[key].read_typetree()
        return self._tt[key]

    def name_of(self, key):
        o = self.objs[key]
        if o.type.name in ("Shader",):
            return o.read().m_ParsedForm.m_Name
        if o.type.name in ("Transform", "MeshFilter", "MeshRenderer", "MonoBehaviour", "BoxCollider", "SkinnedMeshRenderer"):
            return ""
        try:
            return self.tt(key).get("m_Name", "")
        except Exception:  # noqa: BLE001
            return ""

    # ---- PPtr ---------------------------------------------------------------------------------------------------------------
    def ptr(self, src_file, p):
        """Key of a PPtr in dict form {m_FileID, m_PathID} found in an object of `src_file`; None when null."""
        if p is None or p["m_PathID"] == 0:
            return None
        if p["m_FileID"] == 0:
            return (src_file, p["m_PathID"])
        ext = self.files[src_file].externals
        if not 1 <= p["m_FileID"] <= len(ext):
            return None      # not a real PPtr (a struct that merely has these field names, or a script reference)
        return (os.path.basename(ext[p["m_FileID"] - 1].path), p["m_PathID"])

    # ---- MonoBehaviour --------------------------------------------------------------------------------------------------------
    def mb_class(self, key):
        if key not in self._cls:
            try:
                s = self.objs[key].parse_monobehaviour_head().m_Script.read()
                self._cls[key] = (s.m_Namespace + "." if s.m_Namespace else "") + s.m_ClassName
                asm = s.m_AssemblyName
                self._asm[key] = asm[:-4] if asm.endswith(".dll") else asm
            except Exception:  # noqa: BLE001
                self._cls[key] = "?"
                self._asm[key] = "Assembly-CSharp"
        return self._cls[key]

    def mb_fields(self, key, cls=None):
        """Serialized fields of a MonoBehaviour (dict form). The python reader: the generator types a string[] as a string."""
        cls = cls or self.mb_class(key)
        asm = self._asm.get(key) or (self.mb_class(key) and self._asm[key])
        if (asm, cls) not in self._mb_nodes:
            nodes = self.gen.get_nodes_up(asm, cls)
            self._fix_string_arrays(nodes)
            self._mb_nodes[(asm, cls)] = nodes
        o = self.objs[key]
        rd = EndianBinaryReader(o.get_raw_data(), "<")
        return TH.read_typetree(self._mb_nodes[(asm, cls)], rd, True, None, False, o.assets_file)

    def _fix_string_arrays(self, n):
        if n.m_Type == "string" and n.m_Children and n.m_Children[0].m_Name == "Array":
            arr = n.m_Children[0]
            if len(arr.m_Children) == 2 and arr.m_Children[1].m_Type == "string":
                n.m_Type = "vector"
        for c in n.m_Children:
            self._fix_string_arrays(c)

    # ---- GameObject tree ------------------------------------------------------------------------------------------------------
    def tag_name(self, t):
        if t in BUILTIN_TAGS:
            return BUILTIN_TAGS[t]
        if t >= 20000 and t - 20000 < len(self.tags):
            return self.tags[t - 20000]
        return "#%d" % t

    def components(self, go_key):
        go = self.tt(go_key)
        out = []
        for c in go["m_Component"]:
            ck = self.ptr(go_key[0], c["component"])
            if ck is not None and ck in self.objs:
                out.append((self.objs[ck].type.name, ck))
        return out

    def transform_of(self, go_key):
        for n, k in self.components(go_key):
            if n in ("Transform", "RectTransform"):
                return k
        return None

    def children(self, go_key):
        """GameObject keys of the transform children, in m_Children order."""
        tk = self.transform_of(go_key)
        out = []
        for ch in self.tt(tk)["m_Children"]:
            ck = self.ptr(go_key[0], ch)
            out.append(self.ptr(ck[0], self.tt(ck)["m_GameObject"]))
        return out

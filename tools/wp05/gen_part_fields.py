"""Generate include/ksp/parts/part_fields.inc and include/ksp/parts/part_enums.inc from the decompiled C#.

    python -I tools/wp05/gen_part_fields.py <core dir>

part_fields.inc: every public instance field declared in Part.cs (one tab of indent, i.e. the Part class itself), as
    KSP_PART_FIELD(name, kind, cppType, csTypeName, enumTable, defaultExpr, line)
  kind: Single Double Int32 UInt32 Boolean String Vector3 Vector3d Quaternion Color Enum AttachRules StringList Object.
  `Object` fields (references to Unity/KSP objects, other lists) have no storage in kspcore's PartFields (their
  value on a freshly compiled prefab is null/empty and ApplyPartValue cannot set them: no Parse(string)).
part_enums.inc: member tables (declaration order, values) of the enums used by Part/AvailablePart/PartResource fields:
    KSP_ENUM_BEGIN(cppName, csName, isFlags, file, line) KSP_ENUM_MEMBER(cppName, name, value) ... KSP_ENUM_END(cppName)
Regenerate, never hand-edit. Source: decompiled/KSP-Decompiled/Assembly-CSharp (paths relative to it in comments).
"""
import os
import re
import sys

ENUMS = [  # (cppName, file, csName as declared)
    ("AttachModes", "AttachModes.cs", "AttachModes"),
    ("AttachNodeMethod", "AttachNodeMethod.cs", "AttachNodeMethod"),
    ("Part_AutoStrutMode", "Part.cs", "AutoStrutMode"),
    ("Part_DragModel", "Part.cs", "DragModel"),
    ("Part_HighlightType", "Part.cs", "HighlightType"),
    ("PartStates", "PartStates.cs", "PartStates"),
    ("Part_PhysicalSignificance", "Part.cs", "PhysicalSignificance"),
    ("Part_ShowRigidAttachmentOption", "Part.cs", "ShowRigidAttachmentOption"),
    ("SymmetryMethod", "SymmetryMethod.cs", "SymmetryMethod"),
    ("Vessel_ControlLevel", "Vessel.cs", "ControlLevel"),
    ("VesselType", "VesselType.cs", "VesselType"),
    ("PartCategories", "PartCategories.cs", "PartCategories"),
    ("PreferredInitialStage", "PreferredInitialStage.cs", "PreferredInitialStage"),
    ("ResourceFlowMode", "ResourceFlowMode.cs", "ResourceFlowMode"),
    ("ResourceTransferMode", "ResourceTransferMode.cs", "ResourceTransferMode"),
    ("PartResource_FlowMode", "PartResource.cs", "FlowMode"),
    ("AttachNode_NodeType", "AttachNode.cs", "NodeType"),
    ("CompoundPart_AttachState", "CompoundPart.cs", "AttachState"),
]
ENUM_BY_CS = {"AttachModes": "AttachModes", "AttachNodeMethod": "AttachNodeMethod", "AutoStrutMode": "Part_AutoStrutMode",
              "DragModel": "Part_DragModel", "HighlightType": "Part_HighlightType", "PartStates": "PartStates",
              "PhysicalSignificance": "Part_PhysicalSignificance",
              "ShowRigidAttachmentOption": "Part_ShowRigidAttachmentOption", "SymmetryMethod": "SymmetryMethod",
              "Vessel.ControlLevel": "Vessel_ControlLevel", "VesselType": "VesselType",
              "AttachState": "CompoundPart_AttachState"}
TYPE_NAME = {"float": "Single", "double": "Double", "int": "Int32", "uint": "UInt32", "bool": "Boolean",
             "string": "String", "Vector3": "Vector3", "Vector3d": "Vector3d", "Quaternion": "Quaternion",
             "Color": "Color", "AttachRules": "AttachRules"}
KIND = {"float": ("Single", "float"), "double": ("Double", "double"), "int": ("Int32", "int32_t"),
        "uint": ("UInt32", "uint32_t"), "bool": ("Boolean", "bool"), "string": ("String", "NullableString"),
        "Vector3": ("Vector3", "Vector3f"), "Vector3d": ("Vector3d", "Vector3d"),
        "Quaternion": ("Quaternion", "Quaternionf"), "Color": ("Color", "Color"),
        "AttachRules": ("AttachRules", "NullableAttachRules"), "List<string>": ("StringList", "NullableStringList")}


def parse_enum(path, name):
    lines = open(path, encoding="utf-8-sig").read().splitlines()
    for i, l in enumerate(lines):
        if re.match(r"^\s*(public |internal |private )?enum %s\b" % re.escape(name), l):
            flags = i > 0 and "[Flags]" in lines[i - 1]
            j = i + 1
            while "{" not in lines[j]:
                j += 1
            members, val = [], -1
            for l2 in lines[j + 1:]:
                s = l2.strip()
                if s.startswith("}"):
                    break
                if not s or s.startswith("["):
                    continue
                s = s.rstrip(",")
                if "=" in s:
                    n, v = [x.strip() for x in s.split("=", 1)]
                    try:
                        val = int(v.rstrip("u"), 0)
                    except ValueError:
                        val = eval(v, {}, {m: x for m, x in members})
                else:
                    n = s
                    val += 1
                members.append((n, val))
            return flags, members, i + 1
    raise SystemExit("enum %s not found in %s" % (name, path))


def cpp_default(t, init, enum_members):
    if init is None:
        if enum_members is not None:
            return "0"
        return {"float": "0.0f", "double": "0.0", "int": "0", "uint": "0u", "bool": "false", "string": "std::nullopt",
                "Vector3": "Vector3f{}", "Vector3d": "Vector3d{}", "Quaternion": "Quaternionf{0.0f, 0.0f, 0.0f, 0.0f}",
                "Color": "Color{}", "AttachRules": "std::nullopt", "List<string>": "std::nullopt"}[t]
    init = init.strip()
    if enum_members is not None:
        return str(dict(enum_members)[init.split(".")[-1]])
    if t == "float":
        v = init.rstrip("fF")
        if "." not in v and "E" not in v and "e" not in v:
            v += ".0"
        return v + "f"
    if t in ("double", "int", "uint", "bool"):
        return init
    if t == "string":
        return "std::string(%s)" % init
    if t == "Vector3":
        return {"Vector3.zero": "Vector3f{0.0f, 0.0f, 0.0f}", "Vector3.one": "Vector3f{1.0f, 1.0f, 1.0f}",
                "Vector3.up": "Vector3f{0.0f, 1.0f, 0.0f}"}[init]
    if t == "Vector3d":
        return {"Vector3d.zero": "Vector3d{0.0, 0.0, 0.0}"}[init]
    if t == "Quaternion":
        return {"Quaternion.identity": "Quaternionf{0.0f, 0.0f, 0.0f, 1.0f}"}[init]
    if t == "Color":
        return "Color{}  /* %s */" % init
    raise SystemExit("unsupported initializer %r for %s" % (init, t))


def main(core):
    root = None
    p = os.path.abspath(core)
    for _ in range(6):
        cand = os.path.join(p, "decompiled", "KSP-Decompiled", "Assembly-CSharp")
        if os.path.isdir(cand):
            root = cand
            break
        p = os.path.dirname(p)
    if root is None:
        raise SystemExit("decompiled/KSP-Decompiled/Assembly-CSharp not found above " + core)
    enums = {}
    out_e = ["// Generated by tools/wp05/gen_part_fields.py from decompiled/KSP-Decompiled/Assembly-CSharp. Do not edit.",
             "// KSP_ENUM_BEGIN(cppName, csName, isFlags, file, line) KSP_ENUM_MEMBER(cppName, name, value) KSP_ENUM_END(cppName)"]
    for cpp, f, cs in ENUMS:
        flags, members, line = parse_enum(os.path.join(root, f), cs)
        enums[cpp] = members
        out_e.append('KSP_ENUM_BEGIN(%s, "%s", %s, "%s", %d)' % (cpp, cs, "true" if flags else "false", f, line))
        for n, v in members:
            out_e.append('KSP_ENUM_MEMBER(%s, "%s", %d)' % (cpp, n, v))
        out_e.append("KSP_ENUM_END(%s)" % cpp)
    pat = re.compile(r"^	public (?!static|const|override|virtual|abstract|event|class|enum|struct|delegate|interface|sealed)"
                     r"(readonly )?([\w.<>\[\], ]+?) (\w+)( = (.*))?;\s*$")

    def fields_of(cs_file, macro):
        src = open(os.path.join(root, cs_file), encoding="utf-8-sig").read().splitlines()
        lines = []
        for i, l in enumerate(src):
            m = pat.match(l)
            if not m:
                continue
            t, name, init = m.group(2), m.group(3), m.group(5)
            if t in KIND:
                kind, cpp = KIND[t]
                cs = TYPE_NAME.get(t, "List`1")
                d = cpp_default(t, init, None)
                lines.append('%s(%s, %s, %s, "%s", nullptr, %s, %d)' % (macro, name, kind, cpp, cs, d, i + 1))
            elif t in ENUM_BY_CS:
                e = ENUM_BY_CS[t]
                d = cpp_default(t, init, enums[e])
                lines.append('%s(%s, Enum, int32_t, "%s", &kEnum_%s, %s, %d)' % (macro, name, t.split(".")[-1], e, d, i + 1))
            else:
                cs = t.split("<")[0].split(".")[-1]
                if "<" in t:
                    cs += "`%d" % (t.count(",") + 1)
                lines.append('%s(%s, Object, void, "%s", nullptr, 0, %d)' % (macro, name, cs, i + 1))
        return lines

    out = ["// Generated by tools/wp05/gen_part_fields.py from decompiled/KSP-Decompiled/Assembly-CSharp/Part.cs. Do not edit.",
           "// Every public instance field of Part, in declaration order:",
           "// KSP_PART_FIELD(name, kind, cppType, csTypeName, enumTable, defaultExpr, line)"]
    part_lines = fields_of("Part.cs", "KSP_PART_FIELD")
    out += part_lines
    n = len(part_lines)
    out_c = ["// Generated by tools/wp05/gen_part_fields.py from decompiled/KSP-Decompiled/Assembly-CSharp/CompoundPart.cs. Do not edit.",
             "// The public instance fields CompoundPart (a Part subclass, `module = CompoundPart`) declares itself:",
             "// KSP_COMPOUNDPART_FIELD(name, kind, cppType, csTypeName, enumTable, defaultExpr, line)"]
    out_c += fields_of("CompoundPart.cs", "KSP_COMPOUNDPART_FIELD")
    inc = os.path.join(core, "include", "ksp", "parts")
    os.makedirs(inc, exist_ok=True)
    with open(os.path.join(inc, "part_fields.inc"), "w", newline="\n") as f:
        f.write("\n".join(out) + "\n")
    with open(os.path.join(inc, "compound_part_fields.inc"), "w", newline="\n") as f:
        f.write("\n".join(out_c) + "\n")
    with open(os.path.join(inc, "part_enums.inc"), "w", newline="\n") as f:
        f.write("\n".join(out_e) + "\n")
    print("part fields:", n, "enums:", len(ENUMS))


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else ".")

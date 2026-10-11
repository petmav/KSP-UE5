"""Generates the GameSettings field registry from decompiled/KSP-Decompiled/Assembly-CSharp/GameSettings.cs.

Usage: python -I tools/wp50/gen_game_settings.py <core dir>

Outputs (relative to <core dir>):
  include/ksp/input/game_settings_vars.inc   the static fields of class GameSettings as `inline` variables of namespace
                                             ksp::GameSettings, in C# declaration order, except the names the earlier shims
                                             (util/ksp_util.hpp, vessel/vessel.hpp, `namespace GameSettings { ... }`) already define
  src/input/game_settings_defaults.cpp      GameSettings.SetDefaultValues() (every statement of the C# method, translated 1:1,
                                             cited by C# line) and the public-static-field table (name, kind, address) that
                                             WriteCfg / ParseCfg / LoadLayoutKeyBindings reflect over in C#

Every statement of SetDefaultValues must translate; an unknown shape aborts the generator (nothing is dropped silently).
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..', '..'))
SRC_CANDIDATES = [os.path.join(ROOT, 'decompiled', 'KSP-Decompiled', 'Assembly-CSharp', 'GameSettings.cs'),
                  os.path.join('C:' + os.sep, 'Projects', 'KSP-UE5', 'decompiled', 'KSP-Decompiled', 'Assembly-CSharp', 'GameSettings.cs')]

# C# type -> (C++ type, table kind)
TYPES = {
    'string': ('std::string', 'String'), 'int': ('int', 'Int'), 'float': ('float', 'Float'), 'double': ('double', 'Double'),
    'bool': ('bool', 'Bool'),
    'KeyBinding': ('std::shared_ptr<KeyBinding>', 'KeyBinding'),
    'AxisBinding': ('std::shared_ptr<AxisBinding>', 'AxisBinding'),
    'AxisKeyBindingList': ('std::shared_ptr<AxisKeyBindingList>', 'AxisKeyBindingList'),
    'InputDevices': ('std::shared_ptr<InputDevices>', 'InputDevices'),
    'TrackIR.Settings': ('std::shared_ptr<TrackIRSettings>', 'TrackIR'),
    'List<float>': ('std::shared_ptr<std::vector<float>>', 'FloatList'),
    'List<string>': ('std::vector<std::string>', None),
    'List<Color>': ('std::vector<Color>', None),
    'Color': ('Color', None),
    'ValidatorMode': ('Expansions::Missions::Editor::ValidatorMode', 'ValidatorMode'),
    'TimeWarp.MaxRailsRateMode': ('TimeWarp::MaxRailsRateMode', 'MaxRailsRateMode'),
}


def read_source():
    for p in SRC_CANDIDATES:
        if os.path.exists(p):
            return open(p, encoding='utf-8-sig').read().splitlines()
    raise SystemExit('GameSettings.cs not found')


def existing_shim_names(core):
    """names defined by `namespace GameSettings { inline T NAME ... }` blocks in the earlier shims, with their header."""
    out = {}
    for rel in ('include/ksp/util/ksp_util.hpp', 'include/ksp/vessel/vessel.hpp'):
        text = open(os.path.join(core, rel.replace('/', os.sep)), encoding='utf-8').read()
        for m in re.finditer(r'namespace GameSettings \{(.*?)\n\}', text, re.S):
            for d in re.finditer(r'inline\s+([\w:<>]+)\s+(\w+)\s*(?:=|;)', m.group(1)):
                out[d.group(2)] = (d.group(1), rel[len('include/'):])
    return out


def parse_fields(lines):
    fields = []
    for i, line in enumerate(lines):
        m = re.match(r'^\t(public|internal|private) static ([\w<>\.]+) (\w+)(?: = (.+))?;$', line)
        if not m:
            continue
        acc, ty, name, init = m.groups()
        if name in ('fetch', 'graphicsVersion', 'saveOnGameSave'):
            continue
        fields.append(dict(line=i + 1, access=acc, type=ty, name=name, init=init))
    return fields


def method_body(lines, header_re):
    for i, line in enumerate(lines):
        if re.search(header_re, line):
            depth = 0
            start = None
            for j in range(i, len(lines)):
                depth += lines[j].count('{') - lines[j].count('}')
                if '{' in lines[j] and start is None:
                    start = j
                if start is not None and depth == 0:
                    return start + 1, j  # statement lines are lines[start+1:j]
    raise SystemExit('method not found ' + header_re)


def split_strings(expr):
    """[(is_string, text)] pieces so substitutions never touch string literals"""
    parts = []
    cur = ''
    i = 0
    while i < len(expr):
        c = expr[i]
        if c == '"':
            if cur:
                parts.append((False, cur))
                cur = ''
            j = i + 1
            while expr[j] != '"':
                if expr[j] == '\\':
                    j += 1
                j += 1
            parts.append((True, expr[i:j + 1]))
            i = j + 1
        else:
            cur += c
            i += 1
    if cur:
        parts.append((False, cur))
    return parts


def conv_expr(expr, target_type):
    out = ''
    for is_str, text in split_strings(expr):
        if is_str:
            out += text
            continue
        t = text
        t = re.sub(r'\b(isInverted): ', '', t)
        t = t.replace('TimeWarp.MaxRailsRateMode.', 'TimeWarp::MaxRailsRateMode::')
        t = t.replace('ValidatorMode.', 'Expansions::Missions::Editor::ValidatorMode::')
        for ns in ('KeyCode', 'InputBindingModes', 'ControlTypes'):
            t = t.replace(ns + '.', ns + '::')
        t = re.sub(r'(?<![\w.])(\d+)f\b', r'\1.0f', t)
        t = re.sub(r'(?<![\w.])(-?\d+)f\b', r'\1.0f', t)
        out += t
    m = re.match(r'^new (\w+(?:\.\w+)?)\((.*)\)$', out.strip())
    if m:
        cls, args = m.groups()
        cls = {'TrackIR.Settings': 'TrackIRSettings'}.get(cls, cls)
        return 'std::make_shared<%s>(%s)' % (cls, args)
    return out


def main():
    core = sys.argv[1]
    lines = read_source()
    fields = parse_fields(lines)
    shim = existing_shim_names(core)
    by_name = {f['name']: f for f in fields}

    # ---- variables
    inc = ['// GENERATED by tools/wp50/gen_game_settings.py from decompiled/KSP-Decompiled/Assembly-CSharp/GameSettings.cs -- do not edit.',
           '// The static fields of class GameSettings (declaration order). Names the earlier shims already define (%s) are not repeated here.'
           % ', '.join(sorted(shim)), '']
    for f in fields:
        ty = f['type']
        if ty not in TYPES:
            raise SystemExit('unmapped type ' + ty + ' for ' + f['name'])
        cpp, kind = TYPES[ty]
        f['cpp'], f['kind'] = cpp, kind
        if f['name'] in shim:
            f['shim'] = True
            continue
        init = ''
        if f['init'] is not None:
            init = ' = ' + conv_expr(f['init'], ty)
        elif cpp in ('bool', 'int', 'float', 'double'):
            init = ' = 0' if cpp != 'bool' else ' = false'
        inc.append('inline %s %s%s;  // GameSettings.cs:%d (%s)' % (cpp, f['name'], init, f['line'], f['access']))
    open(os.path.join(core, 'include', 'ksp', 'input', 'game_settings_vars.inc'), 'w', newline='\n').write('\n'.join(inc) + '\n')

    # ---- SetDefaultValues
    a, b = method_body(lines, r'public static void SetDefaultValues\(\)')
    stmts = []
    for n in range(a, b):
        line = lines[n].strip()
        if not line:
            continue
        m = re.match(r'^(\w+) = (.+);$', line)
        if not m:
            raise SystemExit('SetDefaultValues: unhandled statement at %d: %s' % (n + 1, line))
        name, expr = m.groups()
        f = by_name.get(name)
        if f is None:
            raise SystemExit('SetDefaultValues assigns unknown field ' + name)
        if f['type'] == 'List<string>':
            stmts.append('    %s.clear();  // GameSettings.cs:%d  %s = new List<string>()' % (name, n + 1, name))
            continue
        stmts.append('    %s = %s;  // GameSettings.cs:%d' % (name, conv_expr(expr, f['type']), n + 1))
    cpp = ['// GENERATED by tools/wp50/gen_game_settings.py from decompiled/KSP-Decompiled/Assembly-CSharp/GameSettings.cs -- do not edit.',
           '// GameSettings.SetDefaultValues() (cs:%d-%d) and the public static field table.' % (a - 1, b + 1),
           '#include "ksp/input/game_settings.hpp"', '', 'namespace ksp {', '',
           '// GameSettings.cs:%d' % (a - 1),
           'void GameSettings::SetDefaultValues() {']
    cpp += stmts
    cpp += ['}', '']

    # ---- field table (public static fields, declaration order = Type.GetFields() order)
    cpp.append('// Type.GetFields() filtered to public static: the order WriteCfg writes and the names ParseCfg / LoadLayoutKeyBindings look up')
    cpp.append('const std::vector<SettingsField>& GameSettings::Fields() {')
    cpp.append('    static const std::vector<SettingsField> table = {')
    for f in fields:
        if f['access'] != 'public' or f['kind'] is None:
            continue
        cpp.append('        {"%s", SettingsField::Kind::%s, &%s},' % (f['name'], f['kind'], f['name']))
    cpp += ['    };', '    return table;', '}', '', '}  // namespace ksp', '']
    open(os.path.join(core, 'src', 'input', 'game_settings_defaults.cpp'), 'w', newline='\n').write('\n'.join(cpp))
    print('fields', len(fields), 'public table', sum(1 for f in fields if f['access'] == 'public' and f['kind']), 'defaults', len(stmts),
          'shim names', sorted(shim))
    hdrs = sorted(set(h for (_t, h) in shim.values()))
    print('shim headers', hdrs)


main()

"""Generates the C++ GameEvents registry from decompiled/KSP-Decompiled/Assembly-CSharp/GameEvents.cs.

Outputs (paths relative to the kspcore source tree given as argv[1]):
  include/ksp/events/game_event_types.hpp  forward declarations of every KSP/Unity type the events mention
  include/ksp/events/game_events.hpp       struct GameEvents: nested structs, nested classes, every event, FindEvent
  src/events/game_events.cpp               the event objects, defined in GameEvents.cs's textual order

Type mapping (C# -> C++), chosen so that a later work package only has to *define* the forward-declared type:
  class / interface T           -> ns::T*            (reference semantics; `class T;` forward declaration)
  struct T                      -> ns::T              (`struct T;`)
  enum T [: u]                  -> ns::T              (`enum class T : u;` opaque declaration, u = C# underlying type)
  nested type Outer.T           -> ns::Outer_T        (C++ cannot forward-declare a member of an incomplete class;
                                                      the owning package adds `using T = Outer_T;` inside Outer)
  List<T>                       -> std::vector<T'>*   (List is a reference type)
  bool/int/uint/float/double/string -> bool/int/uint32_t/float/double/std::string
  Vector3 / Vector3d / ConfigNode -> ksp::Vector3f / ksp::Vector3d / ksp::ConfigNode* (already ported)
  UnityEngine.Transform/PhysicMaterial/RaycastHit/Space -> ksp::UnityEngine::... (engine types, tags only)
  C# namespace A.B              -> ksp::A::B
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cs_types import build_index, resolve  # noqa: E402

ROOT = os.path.join('C:' + os.sep, 'Projects', 'KSP-UE5')
DECOMP = os.path.join(ROOT, 'decompiled', 'KSP-Decompiled')
SRC = os.path.join(DECOMP, 'Assembly-CSharp', 'GameEvents.cs')

USINGS = ['', 'Contracts', 'Expansions.Missions', 'Expansions.Missions.Adjusters', 'Expansions.Missions.Editor',
          'Expansions.Serenity', 'Expansions.Serenity.DeployedScience.Runtime', 'FinePrint', 'KSP.UI', 'KSP.UI.Screens',
          'KSPAchievements', 'Upgradeables']
PRIMS = {'bool': 'bool', 'int': 'int', 'uint': 'uint32_t', 'float': 'float', 'double': 'double',
         'string': 'std::string', 'long': 'int64_t', 'short': 'int16_t', 'byte': 'uint8_t'}
KNOWN = {'Vector3': 'ksp::Vector3f', 'Vector3d': 'ksp::Vector3d', 'ConfigNode': 'ksp::ConfigNode*'}
UNITY = {'Transform': ('class', None), 'PhysicMaterial': ('class', None), 'RaycastHit': ('struct', None),
         'Space': ('enum', 'int')}
NESTED_GE = {'ExplosionReaction', 'VesselSpawnInfo', 'HostTargetAction', 'FromToAction', 'HostedFromToAction'}
UNDERLYING = {None: 'int', 'int': 'int', 'byte': 'uint8_t', 'sbyte': 'int8_t', 'short': 'int16_t', 'ushort': 'uint16_t',
              'uint': 'uint32_t', 'long': 'int64_t', 'ulong': 'uint64_t'}

idx = build_index([os.path.join(DECOMP, 'Assembly-CSharp'), os.path.join(DECOMP, 'Assembly-CSharp-firstpass')])
fwd = {}  # (cpp_namespace, cpp_name) -> (kind, underlying, cs_qualified)
delegates = {}


def split_args(s):
    parts, lvl, cur = [], 0, ''
    for ch in s:
        if ch == '<':
            lvl += 1
        elif ch == '>':
            lvl -= 1
        if ch == ',' and lvl == 0:
            parts.append(cur.strip())
            cur = ''
        else:
            cur += ch
    if cur.strip():
        parts.append(cur.strip())
    return parts


def cpp_ns(cs_ns):
    return 'ksp' + ''.join('::' + p for p in cs_ns.split('.') if p)


def map_type(cs, inside=True):
    cs = cs.strip()
    m = re.match(r'^([\w\.]+)<(.*)>$', cs)
    if m:
        base, args = m.group(1), [map_type(a, inside) for a in split_args(m.group(2))]
        if base == 'List':
            return 'std::vector<%s>*' % args[0]
        if base in NESTED_GE:
            return '%s<%s>' % (base if inside else 'GameEvents::' + base, ', '.join(args))
        raise SystemExit('unmapped generic ' + cs)
    if cs in PRIMS:
        return PRIMS[cs]
    if cs in KNOWN:
        return KNOWN[cs]
    if cs in NESTED_GE:
        return cs if inside else 'GameEvents::' + cs
    if cs in UNITY:
        kind, under = UNITY[cs]
        fwd[('ksp::UnityEngine', cs)] = (kind, under, 'UnityEngine.' + cs)
        return 'ksp::UnityEngine::' + cs + ('*' if kind == 'class' else '')
    cands = resolve(idx, cs, prefer_ns=USINGS)
    cands = [c for c in cands if c[1] in USINGS or cs.count('.')] or cands
    if not cands:
        raise SystemExit('unresolved type ' + cs)
    kinds = {(c[0], c[1], c[2]) for c in cands}
    if len(kinds) > 1:
        raise SystemExit('ambiguous type %s: %s' % (cs, kinds))
    kind, ns, outer, path, under = cands[0]
    simple = cs.split('.')[-1]
    name = (outer.replace('.', '_') + '_' + simple) if outer else simple
    nsq = cpp_ns(ns)
    if kind == 'delegate':
        ret, params = under
        ptypes = []
        for p in split_args(params):
            ptype = p.rsplit(' ', 1)[0]
            ptypes.append(map_type(ptype, inside))
        return 'ksp::Delegate<%s(%s)>' % (map_type(ret, inside) if ret != 'void' else 'void', ', '.join(ptypes))
    fwd[(nsq, name)] = (kind, under if kind == 'enum' else None, (ns + '.' if ns else '') + (outer + '.' if outer else '') + simple)
    return nsq + '::' + name + ('*' if kind in ('class', 'interface') else '')


# ---- parse GameEvents.cs ---------------------------------------------------------------------------------------------
lines = open(SRC, encoding='utf-8-sig').read().splitlines()
decl = re.compile(r'^\s*(public|internal) (static )?([\w<>,\. \[\]]+?) (\w+) = new ([\w<>,\. \[\]]+?)\((.*)\);\s*$')
cls = re.compile(r'^\s*public (static )?class (\w+)')
items = []  # (lineno, scope tuple, access, is_static, cs_type, member, ctor_args)
scope = []
depth = 0
for i, line in enumerate(lines, 1):
    m = cls.match(line)
    if m and not line.startswith('public class GameEvents'):
        scope.append([m.group(2), depth, False])  # [name, depth before its '{', opened]
    d = decl.match(line)
    if d:
        items.append((i, tuple(s[0] for s in scope), d.group(1), bool(d.group(2)), d.group(3), d.group(4), d.group(6)))
    depth += line.count('{') - line.count('}')
    if scope and depth > scope[-1][1]:
        scope[-1][2] = True
    while scope and scope[-1][2] and depth <= scope[-1][1]:
        scope.pop()

# ---- emit ------------------------------------------------------------------------------------------------------------
def event_cpp(cs_type, inside=True):
    m = re.match(r'^(EventData|EventVoid|EventDataModifier|GameModifiers)(?:<(.*)>)?$', cs_type)
    if not m:
        raise SystemExit('unknown event type ' + cs_type)
    base, args = m.group(1), m.group(2)
    if base == 'GameModifiers':
        return 'GameModifiers' if inside else 'GameEvents::GameModifiers'
    if base == 'EventVoid':
        return 'EventVoid'
    return '%s<%s>' % (base, ', '.join(map_type(a, inside) for a in split_args(args)))


def ctor_args(cs_args):
    # only string literals appear (`"name"`)
    if not re.fullmatch(r'"[^"]*"', cs_args.strip()) and cs_args.strip():
        raise SystemExit('unexpected ctor args ' + cs_args)
    return cs_args.strip()


body = {}  # scope tuple -> list of member lines
order = []
for (ln, sc, access, static, cs_type, member, args) in items:
    t = event_cpp(cs_type)
    note = '' if access == 'public' else '  // C#: internal'
    if static:
        body.setdefault(sc, []).append('    static %s %s;  // GameEvents.cs:%d%s' % (t, member, ln, note))
    else:
        body.setdefault(sc, []).append('    %s %s{%s};  // GameEvents.cs:%d%s' % (t, member, ctor_args(args), ln, note))
    order.append((ln, sc, static, cs_type, member, args))

n_static = sum(1 for o in order if o[2])
counts = {}
for (_, _, _, cs_type, _, _) in order:
    k = re.sub(r'<.*', '', cs_type)
    counts[k] = counts.get(k, 0) + 1

out_dir = sys.argv[1]
cb = map_type('VesselSpawnDialog.CraftSelectedCallback')  # VesselSpawnInfo members (before the forward declarations)
lsf = map_type('LaunchSiteFacility')
H = []
H.append('// GENERATED by tools/wp03/gen_game_events.py from decompiled/KSP-Decompiled/Assembly-CSharp/GameEvents.cs -- do not edit.')
H.append('// Forward declarations of the KSP and Unity types that GameEvents mentions. Each is a tag until its work package')
H.append('// defines it (same namespace, same name, same class-key / enum underlying type). Nested C# types Outer.T are declared')
H.append('// at namespace scope as Outer_T (C++ cannot forward-declare members of incomplete classes); the owning package adds')
H.append('// `using T = Outer_T;` inside Outer.')
H.append('#pragma once')
H.append('')
H.append('#include <cstdint>')
H.append('')
by_ns = {}
for (nsq, name), (kind, under, csq) in sorted(fwd.items()):
    by_ns.setdefault(nsq, []).append((name, kind, under, csq))
for nsq in sorted(by_ns):
    H.append('namespace %s {' % nsq)
    for name, kind, under, csq in by_ns[nsq]:
        if kind == 'enum':
            H.append('enum class %s : %s;  // C# enum %s' % (name, UNDERLYING[under], csq))
        elif kind == 'struct':
            H.append('struct %s;  // C# struct %s' % (name, csq))
        else:
            H.append('class %s;  // C# %s %s' % (name, kind, csq))
    H.append('}  // namespace %s' % nsq)
    H.append('')
open(os.path.join(out_dir, 'include', 'ksp', 'events', 'game_event_types.hpp'), 'w', encoding='utf-8', newline='\n').write('\n'.join(H))

G = []
G.append('// GENERATED by tools/wp03/gen_game_events.py from decompiled/KSP-Decompiled/Assembly-CSharp/GameEvents.cs -- do not edit.')
G.append('//')
G.append('// KSP\'s global event registry (class GameEvents : GameEventsBase). %d event objects: %s.' % (
    len(order), ', '.join('%s %d' % (k, v) for k, v in sorted(counts.items()))))
G.append('// Every C# member keeps its exact name, nesting (GameEvents::VesselSituation::onLand, GameEvents::Modifiers.FundsGain)')
G.append('// and event-name string. Types: see game_event_types.hpp (class -> pointer, struct/enum -> value).')
G.append('// All events are constructed during static initialisation in GameEvents.cs\'s textual order (C#: GameEvents\'s own')
G.append('// statics at type initialisation, nested static classes on first access; FindEvent therefore also finds nested-class')
G.append('// events that C# would only find after their class was touched). Duplicate event names in GameEvents.cs (the later')
G.append('// declaration wins in FindEvent): onPartCouple/onPartDeCouple, onPartCoupleComplete/onPartDeCoupleComplete,')
G.append('// OnFlightState (OnRevertToLaunchFlightState/OnRevertToPrelaunchFlightState), OnApplicantHired (OnCrewmemberHired/')
G.append('// OnCrewmemberSacked), OnEVAConstructionModePartAttached (Attached/Detached), "Active Node Changed"')
G.append('// (Mission.onActiveNodeChanging/onActiveNodeChanged).')
G.append('#pragma once')
G.append('')
G.append('#include <cstdint>')
G.append('#include <string>')
G.append('#include <vector>')
G.append('')
G.append('#include "ksp/config/config_node.hpp"')
G.append('#include "ksp/core/delegate.hpp"')
G.append('#include "ksp/events/event_data.hpp"')
G.append('#include "ksp/events/game_event_types.hpp"')
G.append('#include "ksp/math/vector3d.hpp"')
G.append('#include "ksp/unity/vector.hpp"')
G.append('')
G.append('namespace ksp::events {')
G.append('')
G.append('struct GameEvents : GameEventsBase {')
G.append('    // GameEvents.cs:18')
G.append('    struct ExplosionReaction {')
G.append('        float distance = 0.0f;')
G.append('        float magnitude = 0.0f;')
G.append('        ExplosionReaction() = default;')
G.append('        ExplosionReaction(float distance_, float magnitude_) : distance(distance_), magnitude(magnitude_) {}')
G.append('    };')
G.append('    // GameEvents.cs:31')
G.append('    template <class A, class B>')
G.append('    struct HostTargetAction {')
G.append('        A host{};')
G.append('        B target{};')
G.append('        HostTargetAction() = default;')
G.append('        HostTargetAction(A host_, B target_) : host(host_), target(target_) {}')
G.append('    };')
G.append('    // GameEvents.cs:44')
G.append('    template <class A, class B>')
G.append('    struct FromToAction {')
G.append('        A from{};')
G.append('        B to{};')
G.append('        FromToAction() = default;')
G.append('        FromToAction(A from_, B to_) : from(from_), to(to_) {}')
G.append('    };')
G.append('    // GameEvents.cs:57')
G.append('    template <class A, class B>')
G.append('    struct HostedFromToAction {')
G.append('        A host{};')
G.append('        B from{};')
G.append('        B to{};')
G.append('        HostedFromToAction() = default;')
G.append('        HostedFromToAction(A host_, B from_, B to_) : host(host_), from(from_), to(to_) {}')
G.append('    };')
G.append('    // GameEvents.cs:73')
G.append('    struct VesselSpawnInfo {')
G.append('        std::string craftSubfolder;')
G.append('        std::string profileName;')
G.append('        %s OnFileSelected;  // VesselSpawnDialog.CraftSelectedCallback' % cb)
G.append('        %s callingFacility = nullptr;' % lsf)
G.append('        VesselSpawnInfo() = default;')
G.append('        VesselSpawnInfo(std::string craftSubfolder_, std::string profileName_, %s OnFileSelected_,' % cb)
G.append('                        %s callingFacility_)' % lsf)
G.append('            : craftSubfolder(std::move(craftSubfolder_)), profileName(std::move(profileName_)),')
G.append('              OnFileSelected(std::move(OnFileSelected_)), callingFacility(callingFacility_) {}')
G.append('    };')
G.append('')
scopes = []
for o in order:
    if o[1] and o[1] not in scopes:
        scopes.append(o[1])
for sc in scopes:
    name = sc[-1]
    first = min(o[0] for o in order if o[1] == sc)
    if name == 'GameModifiers':
        G.append('    // GameEvents.cs:207 (a non-static nested class; GameEvents.Modifiers is its one instance)')
        G.append('    class GameModifiers {')
        G.append('    public:')
    else:
        G.append('    // GameEvents.cs: public static class %s (first event at line %d)' % (name, first))
        G.append('    struct %s {' % name)
    G.extend(['    ' + l for l in body[sc]])
    G.append('    };')
    G.append('')
G.extend(body.get((), []))
G.append('')
G.append('    // GameEvents.cs:1053')
G.append('    template <class T>')
G.append('    static T* FindEvent(const std::string& eventName) {')
G.append('        return BaseGameEvent::FindEvent<T>(eventName);')
G.append('    }')
G.append('};')
G.append('')
G.append('}  // namespace ksp::events')
G.append('')
G.append('namespace ksp {')
G.append('using events::GameEvents;')
G.append('}  // namespace ksp')
G.append('')
open(os.path.join(out_dir, 'include', 'ksp', 'events', 'game_events.hpp'), 'w', encoding='utf-8', newline='\n').write('\n'.join(G))

C = []
C.append('// GENERATED by tools/wp03/gen_game_events.py from decompiled/KSP-Decompiled/Assembly-CSharp/GameEvents.cs -- do not edit.')
C.append('// Static event objects, defined in GameEvents.cs\'s textual order (= registration order of the event names).')
C.append('#include "ksp/events/game_events.hpp"')
C.append('')
C.append('namespace ksp::events {')
C.append('')
for (ln, sc, static, cs_type, member, args) in order:
    if not static:
        continue
    qual = 'GameEvents::' + ''.join(s + '::' for s in sc) + member
    t = event_cpp(cs_type, inside=False)
    if cs_type == 'GameModifiers':
        C.append('%s %s;  // GameEvents.cs:%d' % (t, qual, ln))
    else:
        C.append('%s %s(%s);  // GameEvents.cs:%d' % (t, qual, ctor_args(args), ln))
C.append('')
C.append('}  // namespace ksp::events')
C.append('')
open(os.path.join(out_dir, 'src', 'events', 'game_events.cpp'), 'w', encoding='utf-8', newline='\n').write('\n'.join(C))

n_events = sum(1 for o in order if o[3] != 'GameModifiers')
print('events: %d (static %d, GameModifiers members %d); kinds %s; forward-declared types %d' % (
    n_events, n_static - 1, len(order) - n_static, counts, len(fwd)))

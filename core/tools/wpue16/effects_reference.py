#!/usr/bin/env python
"""WP-UE-16a: independent Python reference for the EFFECTS evaluation of kspcore (tests/test_effects.cpp).

Written from the decompiled C# (EffectList.cs, EffectBehaviour.cs, FXCurve.cs, FXGroup.cs, AudioFX.cs, AudioLoopFX.cs, AudioSingleFX.cs,
AudioMultiFX.cs, AudioMultiPooledFX.cs, ModelParticleFX.cs, ModelMultiParticleFX.cs, PrefabParticleFX.cs, PrefabMultiParticleFX.cs, the animation
effects, PrefabSpawnFX.cs, ParticleConfigFX.cs, ModuleEnginesFX.cs, ModuleRCSFX.cs); shares no code with the C++.  Mono float rule: C# float
expressions are evaluated in double and rounded to float32 where the source stores / passes / returns; Unity's AnimationCurve is native code
(float32 per operation).

It parses the EFFECTS nodes of every stock part cfg under <mirror>/GameData itself, builds the effect lists, and replays scripted sequences
(power sweeps over every effect, transform indices, one-shot events, pause / unpause, EVA, scripted engine and RCS sequences, legacy FXGroups),
hashing the published state after every step (FNV-1a 64 over canonical text lines; bit patterns for floats).

    python -I tools/wpue16/effects_reference.py --mirror testdata/GameData-mirror > tests/data/wpue16_reference.json
    python -I tools/wpue16/effects_reference.py --mirror testdata/GameData-mirror --dump <part-relpath> <scenario>   (rows of every step)

Fixtures (labelled; the inputs the game's assets would provide): transform counts / activity, clip existence and length, particle base values
and prefab lookups are hash functions of the names (see FIX_*), because the mirror's .mu / prefab / audio files are stubs.
"""
import ctypes
import json
import math
import os
import struct
import sys

# ------------------------------------------------------------------------------------------------------------------------------
# float helpers


ROWS = [0]  # rows hashed (the count STATUS reports)


def f32(x):
    return ctypes.c_float(x).value


def bits(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def fnv64(data, h=0xcbf29ce484222325):
    for b in data:
        h ^= b
        h = (h * 0x100000001b3) & 0xFFFFFFFFFFFFFFFF
    return h


def fnv_str(s):
    return fnv64(s.encode('utf-8'))


def fparse(s):
    return f32(float(s))


# ------------------------------------------------------------------------------------------------------------------------------
# cfg parsing (KSP ConfigNode text rules: // comments, trimmed lines, braces on their own lines, first '=' splits)


class Node:
    def __init__(self, name):
        self.name = name
        self.values = []
        self.nodes = []

    def get(self, key):
        for k, v in self.values:
            if k == key:
                return v
        return None

    def getall(self, key):
        return [v for k, v in self.values if k == key]


def parse_cfg(text):
    lines = []
    for raw in text.replace('\r', '\n').split('\n'):
        s = raw.split('//')[0].strip()
        if not s:
            continue
        # split braces out of the line
        cur = ''
        for ch in s:
            if ch in '{}' and '=' not in cur:
                if cur.strip():
                    lines.append(cur.strip())
                lines.append(ch)
                cur = ''
            else:
                cur += ch
        if cur.strip():
            lines.append(cur.strip())
    root = Node('root')
    stack = [root]
    pending = None
    for ln in lines:
        if ln == '{':
            n = Node(pending if pending is not None else '')
            stack[-1].nodes.append(n)
            stack.append(n)
            pending = None
        elif ln == '}':
            if len(stack) > 1:
                stack.pop()
            else:
                break
            pending = None
        elif '=' in ln:
            k, v = ln.split('=', 1)
            stack[-1].values.append((k.strip(), v.strip()))
            pending = None
        else:
            pending = ln
    return root


# ------------------------------------------------------------------------------------------------------------------------------
# AnimationCurve (Unity native: AnimationCurveTpl<float>), keys with zero tangents, ClampForever


class AnimCurve:
    def __init__(self):
        self.keys = []  # (time, value) sorted, unique times

    def add_key(self, t, v):
        for kt, _ in self.keys:
            if kt == t:
                return -1
        i = 0
        while i < len(self.keys) and self.keys[i][0] < t:
            i += 1
        self.keys.insert(i, (t, v))
        return i

    def evaluate(self, t):
        n = len(self.keys)
        if n == 0:
            return 0.0
        if n == 1:
            return self.keys[0][1]
        last_t, last_v = self.keys[-1]
        if t >= last_t:
            return last_v
        if self.keys[0][0] > t:
            return self.keys[0][1]
        lhs = 0
        for i in range(n):
            if self.keys[i][0] > t:
                break
            lhs = i
        rhs = min(lhs + 1, n - 1)
        lt, lv = self.keys[lhs]
        rt, rv = self.keys[rhs]
        dx = f32(rt - lt)
        dy = f32(rv - lv)
        length = f32(1e-4) if f32(1e-4) > dx else dx
        d0 = f32(length * 0.0)
        d1 = f32(length * 0.0)
        length2 = f32(length * length)
        inv = f32(1.0 / length2)
        a = f32(d1 + d0)
        a = f32(a - dy)
        a = f32(a - dy)
        a = f32(a * inv)
        a = f32(a / length)
        b = f32(dy + dy)
        b = f32(b + dy)
        b = f32(b - d0)
        b = f32(b - d0)
        b = f32(b - d1)
        b = f32(b * inv)
        c = 0.0
        d = lv
        x = f32(t - lt)
        r = f32(x * a)
        r = f32(r + b)
        r = f32(r * x)
        r = f32(r + c)
        r = f32(r * x)
        r = f32(r + d)
        return r


def split_tokens(s):
    out = []
    cur = ''
    for ch in s:
        if ch in ' ,;\t':
            if cur:
                out.append(cur)
            cur = ''
        else:
            cur += ch
    if cur:
        out.append(cur)
    return out


class FXCurve:
    def __init__(self, value):  # the C# constructor ignores the value name
        self.single = value
        self.last = value
        self.eval_single = True
        self.value_name = ''
        self.keys = []
        self.curve = None

    def load(self, name, node):
        self.value_name = name
        vals = node.getall(name)
        if len(vals) == 0:
            self.curve = None
            self.eval_single = True
            return
        if len(vals) == 1:
            arr = split_tokens(vals[0])
            self.single = fparse(arr[0])
            self.last = self.single
            self.curve = None
            self.eval_single = True
            return
        self.keys = []
        self.curve = None
        for v in vals:
            arr = split_tokens(v)
            if len(arr) < 2:
                continue
            self.keys.append((fparse(arr[0]), fparse(arr[1])))
        self.eval_single = False
        self.compile()

    def compile(self):
        c = AnimCurve()
        for t, v in self.keys:
            c.add_key(t, v)
        self.curve = c

    def value(self, t):
        if self.eval_single:
            return self.single
        if self.curve is None:
            self.compile()
        return self.curve.evaluate(t)

    def save(self, out):
        if self.eval_single:
            out.append((self.value_name, g7(self.single)))
            return
        for t, v in self.keys:
            out.append((self.value_name, g7(t) + ' ' + g7(v)))


def g_fmt(x, prec):
    if x != x:
        return 'NaN'
    if math.isinf(x):
        return 'Infinity' if x > 0 else '-Infinity'
    s = '%.*g' % (prec, x)
    if 'e' in s:
        m, e = s.split('e')
        sign = e[0]
        digits = e[1:].lstrip('0')
        digits = digits.rjust(2, '0')
        s = m + 'E' + sign + digits
    return s


def g7(x):
    return g_fmt(x, 7)


def g9(x):
    return g_fmt(x, 9)


# ------------------------------------------------------------------------------------------------------------------------------
# fixtures (labelled test inputs): see the module docstring


def FIX_count(part, tname):
    return fnv_str(part + '|' + tname) % 4


def FIX_active(part, tname, idx):
    return fnv_str('%s|%s|%d' % (part, tname, idx)) % 5 != 0


def FIX_clip_exists(clip):
    return fnv_str(clip) % 13 != 0


def FIX_clip_length(clip):
    return f32(0.25 * (1 + fnv_str(clip) % 16))


def FIX_model(name):
    h = fnv_str(name)
    found = h % 23 != 0
    has_emitter = h % 29 != 0
    mn = float(10 + h % 50)
    mx = mn + float((h >> 8) % 20)
    en0 = f32(0.1 * (1 + (h >> 16) % 9))
    en1 = f32(en0 + 0.5)
    vel = (f32(((h >> 24) % 5) * 0.25), f32(((h >> 28) % 5) * -0.5), f32(((h >> 32) % 7) * 0.5))
    return found, has_emitter, mn, mx, en0, en1, vel


def FIX_prefab(name):
    h = fnv_str(name)
    found = h % 17 != 0
    has_ps = h % 19 != 0
    burst = (h >> 3) & 1
    rate = float(20 + (h >> 5) % 40)
    l0 = f32(0.2 * (1 + (h >> 11) % 6))
    l1 = f32(l0 + 0.75)
    vel = (f32(((h >> 24) % 5) * 0.25), f32(((h >> 28) % 5) * -0.5), f32(((h >> 32) % 7) * 0.5))
    return found, has_ps, burst, rate, l0, l1, vel


def FIX_prefab_exists(name):
    return fnv_str(name) % 17 != 0


def FIX_anim_found(name, anim, clip):
    return fnv_str(name + '|' + anim + '|' + clip) % 5 != 0


def FIX_part_anim(part, tname, clip):
    return fnv_str(part + '|' + tname + '|' + clip) % 3 != 0


VOL = {'Ship': f32(0.75), 'Voice': f32(0.5), 'Ambient': f32(0.625), 'Music': f32(0.4), 'UI': f32(0.9)}
CHANNELS = ['Ship', 'Voice', 'Ambient', 'Music', 'UI']
ANIMTYPES = ['HoldValue', 'Play']

# ------------------------------------------------------------------------------------------------------------------------------
# persistent field loading (ConfigNode.ReadObject)


def p_str(node, name, cur):
    for k, v in node.values:
        if k == name:
            cur = v
    return cur


def p_bool(node, name, cur):
    for k, v in node.values:
        if k == name:
            t = v.strip().lower()
            if t == 'true':
                cur = True
            elif t == 'false':
                cur = False
    return cur


def p_int(node, name, cur):
    for k, v in node.values:
        if k == name:
            try:
                cur = int(v.strip())
            except ValueError:
                pass
    return cur


def p_float(node, name, cur):
    for k, v in node.values:
        if k == name:
            try:
                cur = f32(float(v.strip()))
            except ValueError:
                pass
    return cur


def p_vec(node, name, cur, n):
    for k, v in node.values:
        if k == name:
            arr = [x for x in v.replace(',', ' ').replace('\t', ' ').split(' ') if x]
            if len(arr) != n:
                cur = tuple([0.0] * n)
            else:
                cur = tuple(fparse(x) for x in arr)
    return cur


def p_enum(node, name, cur, names):
    for k, v in node.values:
        if k == name:
            t = v.strip()
            if t in names:
                cur = names.index(t)
            else:
                cur = int(t)
    return cur


def w_vec(v):
    return ','.join(g9(x) for x in v)


def floor_int(x):
    return int(math.floor(x))


def prod(a, b):
    return f32(a * b)


def short(i):
    i &= 0xFFFF
    return i - 0x10000 if i >= 0x8000 else i


def change_ext(path):
    dot = path.rfind('.')
    sep = max(path.rfind('/'), path.rfind('\\'), path.rfind(':'))
    if dot < 0 or (sep >= 0 and dot < sep):
        return path
    return path[:dot]


# ------------------------------------------------------------------------------------------------------------------------------
# audio sources


class Src:
    def __init__(self, env):
        self.id = env.next_id()
        self.clip = ''
        self.loop = False
        self.volume = 1.0
        self.pitch = 1.0
        self.spatial = 0.0
        self.maxdist = 500.0
        self.playing = False
        self.paused = False
        self.tidx = -1
        self.elapsed = 0.0

    def play(self):
        self.playing = True
        self.paused = False
        self.elapsed = 0.0

    def stop(self):
        self.playing = False
        self.paused = False

    def advance(self, env, dt):
        if not self.playing or self.loop:
            return
        self.elapsed = f32(self.elapsed + dt)
        if self.elapsed >= FIX_clip_length(self.clip):
            self.playing = False

    def row(self):
        return 'src %d|%s|%d|%08x|%08x|%08x|%08x|%d|%d|%d|%08x' % (
            self.id, self.clip, 1 if self.loop else 0, bits(self.volume), bits(self.pitch), bits(self.spatial), bits(self.maxdist),
            1 if self.playing else 0, 1 if self.paused else 0, self.tidx, bits(self.elapsed))


def set_src_volume(src, vol_set, channel):
    src.volume = f32(VOL[CHANNELS[channel]] * vol_set)


class Env:
    def __init__(self, part, eva=False, scene_loading=False):
        self.part = part
        self.eva = eva
        self.scene_loading = scene_loading
        self.ids = 0
        self.events = []
        self.part_audio = None
        self.pool_alloc = 0
        self.pool_max = 0
        self.pool_free = []
        self.pool_arena = []

    def next_id(self):
        self.ids += 1
        return self.ids

    def get_part_audio(self):
        if self.part_audio is None:
            self.part_audio = Src(self)
            self.ids -= 1  # the part's own AudioSource does not consume a source id
            self.part_audio.id = 0
        return self.part_audio

    def event(self, beh, kind, tidx, emi, has_power, power):
        self.events.append((kind, beh.effect_name, beh.instance_name if beh.instance_name is not None else '', beh.NODE, tidx, emi, 1 if has_power else 0, power))

    def count(self, tname):
        return FIX_count(self.part, tname)


# ------------------------------------------------------------------------------------------------------------------------------
# effect classes


class Beh:
    NODE = ''

    def __init__(self):
        self.effect_name = ''
        self.instance_name = ''
        self.env = None
        self.subscribed = False

    # event dispatch of EffectBehaviour
    def on_event_idx(self, idx):
        self.on_event()

    def on_event(self):
        pass

    def on_event_power(self, power):
        pass

    def on_event_power_idx(self, power, idx):
        self.on_event_power(power)

    def update(self, dt):
        pass

    def pause(self):
        pass

    def unpause(self):
        pass

    def initialize(self):
        pass

    def inst_row(self):
        return 'inst %s|%s|%s' % (self.effect_name, self.instance_name if self.instance_name is not None else '', self.NODE)


def fl(name, v):
    return 'f %s %08x' % (name, bits(v))


def st(name, v):
    return 's %s=%s' % (name, v)


class AudioFX(Beh):
    NODE = 'AUDIO'

    def __init__(self):
        super().__init__()
        self.channel = 0
        self.loop = False
        self.polyphony = 1
        self.clip = ''
        self.maxdist = 500.0
        self.volume = FXCurve(1.0)
        self.pitch = FXCurve(1.0)
        self.vol_set = 0.0
        self.has_clip = False
        self.clip_name = ''
        self.sources = []
        self.paused = False

    def load(self, node):
        self.channel = p_enum(node, 'channel', self.channel, CHANNELS)
        self.loop = p_bool(node, 'loop', self.loop)
        self.polyphony = p_int(node, 'polyphony', self.polyphony)
        self.clip = p_str(node, 'clip', self.clip)
        self.maxdist = p_float(node, 'maxVolumeDistance', self.maxdist)
        self.volume.load('volume', node)
        self.pitch.load('pitch', node)

    def save(self):
        out = [('channel', CHANNELS[self.channel]), ('loop', 'True' if self.loop else 'False'), ('polyphony', str(self.polyphony)), ('clip', self.clip),
               ('maxVolumeDistance', g9(self.maxdist))]
        self.volume.save(out)
        self.pitch.save(out)
        return out

    def initialize(self):
        self.has_clip = FIX_clip_exists(self.clip)
        self.clip_name = self.clip
        if not self.env.scene_loading:
            self.subscribed = True

    def on_event_power(self, power):
        self.play(power)

    def on_event(self):
        self.play(1.0)

    def create_source(self):
        s = Src(self.env)
        s.clip = self.clip_name
        s.volume = self.volume.last
        s.pitch = self.pitch.last
        s.loop = self.loop
        s.maxdist = self.maxdist
        self.sources.append(s)
        return s

    def play(self, power):
        if not self.has_clip:
            if power <= 0.0:
                return
            url = change_ext(self.clip)
            self.has_clip = FIX_clip_exists(url)
            if not self.has_clip:
                return
            self.clip_name = url
        if not self.env.eva:
            if len(self.sources) >= max(self.polyphony, 0):
                if not self.sources:
                    raise IndexError('sources[0]')
                src = self.sources[0]
            else:
                src = self.create_source()
        else:
            src = self.env.get_part_audio()
            src.clip = self.clip_name
        self.vol_set = self.volume.value(power)
        set_src_volume(src, self.vol_set, self.channel)
        src.pitch = self.pitch.value(power)
        src.spatial = 1.0
        if not self.paused and (not src.playing or not self.loop):
            src.play()
            self.env.event(self, 'audioPlay', -1, src.id, True, power)

    def update(self, dt):
        if not self.paused:
            for s in self.sources:
                s.advance(self.env, dt)
        if not self.sources or self.paused:
            return
        n = len(self.sources)
        while n > 0:
            n -= 1
            s = self.sources[n]
            if not s.playing:
                del self.sources[n]
            else:
                set_src_volume(s, self.vol_set, self.channel)

    def pause(self):
        self.paused = True
        for s in self.sources:
            s.paused = True
            s.playing = False

    def unpause(self):
        self.paused = False
        for s in self.sources:
            s.play()

    def rows(self):
        r = [fl('volumeSet', self.vol_set), fl('isPaused', 1.0 if self.paused else 0.0), fl('channel', float(self.channel)), fl('loop', 1.0 if self.loop else 0.0),
             fl('polyphony', float(self.polyphony)), fl('maxVolumeDistance', self.maxdist), st('clip', self.clip)]
        r += [s.row() for s in self.sources]
        return r


class AudioLoopFX(Beh):
    NODE = 'AUDIO_LOOP'

    def __init__(self):
        super().__init__()
        self.channel = 0
        self.clip = ''
        self.volume = FXCurve(1.0)
        self.pitch = FXCurve(1.0)
        self.vol_set = 0.0
        self.has_clip = False
        self.src = None
        self.paused = False

    def load(self, node):
        self.channel = p_enum(node, 'channel', self.channel, CHANNELS)
        self.clip = p_str(node, 'clip', self.clip)
        self.volume.load('volume', node)
        self.pitch.load('pitch', node)

    def save(self):
        out = [('channel', CHANNELS[self.channel]), ('clip', self.clip)]
        self.volume.save(out)
        self.pitch.save(out)
        return out

    def initialize(self):
        self.has_clip = FIX_clip_exists(self.clip)
        if not self.env.scene_loading:
            self.subscribed = True

    def on_event_power(self, power):
        self.play(power)

    def on_event(self):
        self.play(1.0)

    def play(self, power):
        if not self.has_clip:
            return
        if self.src is None:
            s = Src(self.env)
            s.clip = self.clip
            s.volume = self.volume.last
            s.pitch = self.pitch.last
            s.loop = True
            self.src = s
        self.vol_set = self.volume.value(power)
        set_src_volume(self.src, self.vol_set, self.channel)
        self.src.pitch = self.pitch.value(power)
        self.src.spatial = 1.0
        if not self.src.playing and not self.paused:
            self.src.play()
            self.env.event(self, 'audioPlay', -1, self.src.id, True, power)

    def update(self, dt):
        if not self.paused and self.src is not None:
            set_src_volume(self.src, self.vol_set, self.channel)

    def pause(self):
        self.paused = True
        if self.src is not None:
            self.src.paused = True
            self.src.playing = False

    def unpause(self):
        self.paused = False
        if self.src is not None and self.vol_set > 0.0:
            self.src.play()

    def rows(self):
        r = [fl('volumeSet', self.vol_set), fl('isPaused', 1.0 if self.paused else 0.0), fl('channel', float(self.channel)), st('clip', self.clip)]
        if self.src is not None:
            r.append(self.src.row())
        return r


class AudioSingleFX(Beh):
    NODE = 'AUDIO_SINGLE'

    def __init__(self):
        super().__init__()
        self.channel = 0
        self.polyphony = 1
        self.clip = ''
        self.volume = FXCurve(1.0)
        self.pitch = FXCurve(1.0)
        self.vol_set = 0.0
        self.has_clip = False
        self.sources = []
        self.paused = False

    def load(self, node):
        self.channel = p_enum(node, 'channel', self.channel, CHANNELS)
        self.polyphony = p_int(node, 'polyphony', self.polyphony)
        self.clip = p_str(node, 'clip', self.clip)
        self.volume.load('volume', node)
        self.pitch.load('pitch', node)

    def save(self):
        out = [('channel', CHANNELS[self.channel]), ('polyphony', str(self.polyphony)), ('clip', self.clip)]
        self.volume.save(out)
        self.pitch.save(out)
        return out

    def initialize(self):
        self.has_clip = FIX_clip_exists(self.clip)
        if not self.env.scene_loading:
            self.subscribed = True

    def on_event_power(self, power):
        self.play(power)

    def on_event(self):
        self.play(1.0)

    def play(self, power):
        if not self.has_clip:
            return
        if len(self.sources) >= max(self.polyphony, 0):
            if not self.sources:
                raise IndexError('sources[0]')
            src = self.sources[0]
        else:
            src = Src(self.env)
            src.clip = self.clip
            src.volume = self.volume.last
            src.pitch = self.pitch.last
            src.loop = False
            self.sources.append(src)
        self.vol_set = self.volume.value(power)
        set_src_volume(src, self.vol_set, self.channel)
        src.pitch = self.pitch.value(power)
        src.spatial = 1.0
        if not src.playing and not self.paused:
            src.play()
            self.env.event(self, 'audioPlay', -1, src.id, True, power)

    def update(self, dt):
        if not self.paused:
            for s in self.sources:
                s.advance(self.env, dt)
        if not self.sources or self.paused:
            return
        n = len(self.sources)
        while n > 0:
            n -= 1
            s = self.sources[n]
            if not s.playing:
                del self.sources[n]
            else:
                set_src_volume(s, self.vol_set, self.channel)

    def pause(self):
        self.paused = True
        for s in self.sources:
            s.paused = True
            s.playing = False

    def unpause(self):
        self.paused = False
        for s in self.sources:
            s.play()

    def rows(self):
        r = [fl('volumeSet', self.vol_set), fl('isPaused', 1.0 if self.paused else 0.0), fl('channel', float(self.channel)), fl('polyphony', float(self.polyphony)),
             st('clip', self.clip)]
        r += [s.row() for s in self.sources]
        return r


class AudioMultiFX(Beh):
    NODE = 'AUDIO_MULTI'

    def __init__(self):
        super().__init__()
        self.channel = 0
        self.clip = ''
        self.tname = ''
        self.volume = FXCurve(1.0)
        self.pitch = FXCurve(1.0)
        self.has_clip = False
        self.source = None
        self.volumes = None
        self.nparents = 0
        self.paused = False

    def load(self, node):
        self.channel = p_enum(node, 'channel', self.channel, CHANNELS)
        self.clip = p_str(node, 'clip', self.clip)
        self.tname = p_str(node, 'transformName', self.tname)
        self.volume.load('volume', node)
        self.pitch.load('pitch', node)

    def save(self):
        out = [('channel', CHANNELS[self.channel]), ('clip', self.clip), ('transformName', self.tname)]
        self.volume.save(out)
        self.pitch.save(out)
        return out

    def initialize(self):
        self.has_clip = FIX_clip_exists(self.clip)
        self.nparents = self.env.count(self.tname)
        if self.nparents == 0:
            self.has_clip = False
            return
        self.volumes = [0.0] * self.nparents
        if not self.env.scene_loading:
            self.subscribed = True

    def on_event_power_idx(self, power, idx):
        if self.volumes is None:
            return
        if 0 <= idx < len(self.volumes):
            self.play(power, idx)
            return
        for i in range(len(self.volumes)):
            self.play(power, i)

    def on_event_idx(self, idx):
        self.on_event_power_idx(1.0, idx)

    def play(self, power, idx):
        if not self.has_clip:
            return
        if self.source is None:
            s = Src(self.env)
            s.clip = self.clip
            s.volume = self.volume.last
            s.pitch = self.pitch.last
            s.loop = False
            s.tidx = 0
            self.source = s
        self.volumes[idx] = self.volume.value(power)
        num = 0.0
        for v in reversed(self.volumes):
            num = num if num > v else v
        set_src_volume(self.source, num, self.channel)
        self.source.pitch = self.pitch.value(power)
        self.source.spatial = 1.0
        if not self.paused:
            if not self.source.playing and num > 0.0:
                self.source.play()
                self.env.event(self, 'audioPlay', idx, self.source.id, True, power)
            elif self.source.playing and num <= 0.0:
                self.source.stop()
                self.env.event(self, 'audioStop', idx, self.source.id, True, power)

    def update(self, dt):
        if self.source is not None and not self.paused:
            self.source.advance(self.env, dt)
            num = 0.0
            for v in reversed(self.volumes):
                num = num if num > v else v
            set_src_volume(self.source, num, self.channel)

    def pause(self):
        self.paused = True
        if self.source is not None:
            self.source.paused = True
            self.source.playing = False

    def unpause(self):
        self.paused = False
        if self.source is not None:
            self.source.play()

    def rows(self):
        r = [fl('isPaused', 1.0 if self.paused else 0.0), fl('channel', float(self.channel)), fl('transforms', float(self.nparents))]
        for i, v in enumerate(self.volumes or []):
            r.append(fl('volume%d' % i, v))
        r += [st('clip', self.clip), st('transformName', self.tname)]
        if self.source is not None:
            r.append(self.source.row())
        return r


class AudioMultiPooledFX(Beh):
    NODE = 'AUDIO_MULTI_POOL'
    MAXSRC = 64

    def __init__(self):
        super().__init__()
        self.channel = 0
        self.clip = ''
        self.tname = ''
        self.volume = FXCurve(1.0)
        self.pitch = FXCurve(1.0)
        self.has_clip = False
        self.source = -1
        self.volumes = None
        self.parents = []
        self.paused = False

    def load(self, node):
        self.channel = p_enum(node, 'channel', self.channel, CHANNELS)
        self.clip = p_str(node, 'clip', self.clip)
        self.tname = p_str(node, 'transformName', self.tname)
        self.volume.load('volume', node)
        self.pitch.load('pitch', node)
        self.setup_clip()

    def save(self):
        out = [('channel', CHANNELS[self.channel]), ('clip', self.clip), ('transformName', self.tname)]
        self.volume.save(out)
        self.pitch.save(out)
        return out

    def setup_clip(self):
        self.has_clip = FIX_clip_exists(self.clip)
        n = self.env.count(self.tname)
        self.parents = list(range(n))
        if n == 0:
            self.has_clip = False
        else:
            self.volumes = [0.0] * n

    def initialize(self):
        self.setup_clip()
        if not self.env.scene_loading:
            self.subscribed = True

    def on_event_power_idx(self, power, idx):
        if self.volumes is None:
            return
        if 0 <= idx < len(self.volumes):
            self.play(power, idx)
            return
        for i in range(len(self.volumes)):
            self.play(power, i)

    def on_event_idx(self, idx):
        self.on_event_power_idx(1.0, idx)

    def release(self, idx):
        env = self.env
        ps = env.pool_arena[idx]
        owner = ps['owner']
        if owner is not None:
            owner.source = -1
        ps['owner'] = None
        ps['enabled'] = False
        ps['src'].clip = ''
        ps['parented'] = False
        ps['active'] = False
        env.pool_free.append(idx)
        env.pool_max = max(env.pool_max, len(env.pool_free))

    def play(self, power, idx):
        if not self.has_clip:
            return
        env = self.env
        self.volumes[idx] = self.volume.value(power)
        num = 0.0
        for v in reversed(self.volumes):
            num = num if num > v else v
        if self.paused:
            return
        parent = self.parents[0]
        for k in range(len(self.parents)):
            if FIX_active(env.part, self.tname, k):
                parent = self.parents[k]
                break
        if self.source < 0 and num > 0.0:
            if env.pool_max == 0 and env.pool_alloc > self.MAXSRC:
                return
            if env.pool_free:
                i = env.pool_free.pop()
            else:
                env.pool_alloc += 1
                s = Src(env)
                s.volume = 0.0
                s.pitch = 0.0
                s.spatial = 1.0
                s.loop = False
                env.pool_arena.append({'src': s, 'owner': None, 'enabled': False, 'parented': False, 'active': False})
                i = len(env.pool_arena) - 1
            ps = env.pool_arena[i]
            ps['owner'] = self
            ps['parented'] = True
            ps['active'] = True
            ps['enabled'] = True
            ps['src'].clip = self.clip
            ps['src'].pitch = self.pitch.value(power)
            ps['src'].tidx = parent
            set_src_volume(ps['src'], num, self.channel)
            if FIX_active(env.part, self.tname, parent):
                ps['src'].play()
                env.event(self, 'audioPlay', idx, ps['src'].id, True, power)
            self.source = i
        elif self.source >= 0 and num > 0.0:
            ps = env.pool_arena[self.source]
            ps['src'].pitch = self.pitch.value(power)
            set_src_volume(ps['src'], num, self.channel)
        elif self.source >= 0 and num <= 0.0:
            ps = env.pool_arena[self.source]
            ps['src'].stop()
            env.event(self, 'audioStop', idx, ps['src'].id, True, power)
            self.release(self.source)

    def update(self, dt):
        env = self.env
        if self.source >= 0 and not self.paused:
            ps = env.pool_arena[self.source]
            a = 0.0
            for v in reversed(self.volumes):
                a = a if a > v else v
            set_src_volume(ps['src'], a, self.channel)
        if self.source >= 0:
            ps = env.pool_arena[self.source]
            ps['src'].advance(env, dt)
            if not ps['src'].playing and ps['parented']:
                self.release(self.source)

    def pause(self):
        if self.source >= 0:
            ps = self.env.pool_arena[self.source]
            ps['src'].stop()
            self.release(self.source)
            self.paused = True

    def unpause(self):
        if self.paused:
            self.paused = False

    def rows(self):
        r = [fl('isPaused', 1.0 if self.paused else 0.0), fl('channel', float(self.channel)), fl('transforms', float(len(self.parents)))]
        for i, v in enumerate(self.volumes or []):
            r.append(fl('volume%d' % i, v))
        r += [st('clip', self.clip), st('transformName', self.tname)]
        if self.source >= 0:
            r.append(self.env.pool_arena[self.source]['src'].row())
        return r


def em_row(i, e):
    return 'em %d|%d|%d|%d|%d|%d|%d|%08x|%08x|%08x|%08x|%08x|%08x|%08x|%08x' % (
        i, e['emit'], e['playing'], e['burst'], e['resolved'], e['minE'], e['maxE'], bits(e['minEn']), bits(e['maxEn']), bits(e['vx']), bits(e['vy']), bits(e['vz']),
        bits(e['ep']), bits(e['enp']), bits(e['sp']))


def new_em(i):
    return {'emit': 0, 'playing': 0, 'burst': 0, 'resolved': 0, 'minE': 0, 'maxE': 0, 'minEn': 0.0, 'maxEn': 0.0, 'vx': 0.0, 'vy': 0.0, 'vz': 0.0, 'ep': 0.0, 'enp': 0.0, 'sp': 0.0}


class ModelParticleBase(Beh):
    MULTI = False

    def __init__(self):
        super().__init__()
        self.model = ''
        self.tname = ''
        self.rot = (0.0, 0.0, 0.0)
        self.pos = (0.0, 0.0, 0.0)
        self.scl = (1.0, 1.0, 1.0)
        self.emission = FXCurve(1.0)
        self.energy = FXCurve(1.0)
        self.speed = FXCurve(1.0)
        self.init = False
        self.base = None
        self.emitters = []
        self.ep = self.enp = self.sp = 0.0

    def load(self, node):
        self.model = p_str(node, 'modelName', self.model)
        self.tname = p_str(node, 'transformName', self.tname)
        self.rot = p_vec(node, 'localRotation', self.rot, 3)
        self.pos = p_vec(node, 'localPosition', self.pos, 3)
        self.scl = p_vec(node, 'localScale', self.scl, 3)
        self.emission.load('emission', node)
        self.energy.load('energy', node)
        self.speed.load('speed', node)

    def save(self):
        out = [('modelName', self.model), ('transformName', self.tname), ('localRotation', w_vec(self.rot)), ('localPosition', w_vec(self.pos)),
               ('localScale', w_vec(self.scl))]
        self.emission.save(out)
        self.energy.save(out)
        self.speed.save(out)
        return out

    def init_emitters(self, count):
        found, has_em, mn, mx, en0, en1, vel = FIX_model(self.model)
        if not found or not has_em:
            return False
        self.base = (mn, mx, en0, en1, vel)
        for _ in range(count):
            self.emitters.append(new_em(len(self.emitters)))
        self.init = True
        return True

    def has_single_transform(self):
        return self.env.count(self.tname) > 0

    def set_emitter(self, i, minv, maxv, minen, maxen, vel):
        e = self.emitters[i]
        e['emit'] = 1
        e['minE'] = minv
        e['maxE'] = maxv
        e['minEn'] = minen
        e['maxEn'] = maxen
        e['vx'], e['vy'], e['vz'] = vel
        e['resolved'] = 1
        e['ep'] = self.ep
        e['enp'] = self.enp
        e['sp'] = self.sp

    def powered(self, power, idx):
        if not self.init:
            return
        if power <= 0.0:
            if 0 <= idx < len(self.emitters):
                self.emitters[idx]['emit'] = 0
                return
            for e in self.emitters:
                e['emit'] = 0
            return
        self.ep = self.emission.value(power)
        self.enp = self.energy.value(power)
        self.sp = self.speed.value(power)
        mn, mx, en0, en1, vel = self.base
        minv = floor_int(prod(mn, self.ep))
        maxv = floor_int(prod(mx, self.ep))
        minen = prod(en0, self.enp)
        maxen = prod(en1, self.enp)
        v = (prod(vel[0], self.sp), prod(vel[1], self.sp), prod(vel[2], self.sp))
        if 0 <= idx < len(self.emitters):
            self.set_emitter(idx, minv, maxv, minen, maxen, v)
            return
        for j in range(len(self.emitters)):
            self.set_emitter(j, minv, maxv, minen, maxen, v)

    def rows(self):
        r = [st('modelName', self.model), st('transformName', self.tname)]
        for n, v in (('localRotation', self.rot), ('localPosition', self.pos), ('localScale', self.scl)):
            for a, c in zip('xyz', v):
                r.append(fl('%s.%s' % (n, a), c))
        r += [fl('initialized', 1.0 if self.init else 0.0), fl('emissionPower', self.ep), fl('energyPower', self.enp), fl('speedPower', self.sp)]
        r += [em_row(i, e) for i, e in enumerate(self.emitters)]
        return r


class ModelParticleFX(ModelParticleBase):
    NODE = 'MODEL_PARTICLE'

    def initialize(self):
        if not self.has_single_transform():
            return
        self.emitters = []
        self.init_emitters(1)

    def on_event(self):
        if not self.init:
            return
        self.env.event(self, 'emit', -1, 0, False, 0.0)

    def on_event_power(self, power):
        self.powered(power, -1)


class ModelMultiParticleFX(ModelParticleBase):
    NODE = 'MODEL_MULTI_PARTICLE'

    def initialize(self):
        n = self.env.count(self.tname)
        if n == 0:
            return
        self.init_emitters(n)

    def on_event_idx(self, idx):
        if not self.init:
            return
        if 0 <= idx < len(self.emitters):
            self.env.event(self, 'emit', idx, idx, False, 0.0)
            return
        for i in range(len(self.emitters)):
            self.env.event(self, 'emit', idx, i, False, 0.0)

    def on_event_power_idx(self, power, idx):
        self.powered(power, idx)


class PrefabParticleBase(Beh):
    def __init__(self):
        super().__init__()
        self.prefab = ''
        self.tname = ''
        self.emission = FXCurve(1.0)
        self.energy = FXCurve(1.0)
        self.speed = FXCurve(1.0)
        self.off = (0.0, 0.0, 0.0)
        self.rot = (0.0, 0.0, 0.0, 0.0)
        self.scl = (1.0, 1.0, 1.0)
        self.one_shot = False
        self.init = False
        self.bursts = False
        self.base = None
        self.emitters = []
        self.ep = self.enp = self.sp = 0.0

    def load(self, node):
        self.prefab = p_str(node, 'prefabName', self.prefab)
        self.tname = p_str(node, 'transformName', self.tname)
        self.off = p_vec(node, 'localOffset', self.off, 3)
        self.rot = p_vec(node, 'localRotation', self.rot, 4)
        self.scl = p_vec(node, 'localScale', self.scl, 3)
        self.one_shot = p_bool(node, 'oneShot', self.one_shot)
        self.emission.load('emission', node)
        self.energy.load('energy', node)
        self.speed.load('speed', node)

    def save(self):
        out = [('prefabName', self.prefab), ('transformName', self.tname), ('localOffset', w_vec(self.off)), ('localRotation', w_vec(self.rot)),
               ('localScale', w_vec(self.scl)), ('oneShot', 'True' if self.one_shot else 'False')]
        self.emission.save(out)
        self.energy.save(out)
        self.speed.save(out)
        return out

    def init_prefab(self):
        found, has_ps, burst, rate, l0, l1, vel = FIX_prefab(self.prefab)
        if not found or not has_ps:
            return False
        self.bursts = burst > 0
        if self.bursts:
            mn = mx = 0.0
        else:
            mn = mx = rate
        self.base = (mn, mx, l0, l1, vel)
        self.init = True
        return True

    def rows(self):
        r = [st('prefabName', self.prefab), st('transformName', self.tname), fl('oneShot', 1.0 if self.one_shot else 0.0)]
        for n, v in (('localOffset', self.off), ('localRotation', self.rot), ('localScale', self.scl)):
            for a, c in zip('xyzw', v):
                r.append(fl('%s.%s' % (n, a), c))
        r += [fl('initialized', 1.0 if self.init else 0.0), fl('emissionPower', self.ep), fl('energyPower', self.enp), fl('speedPower', self.sp)]
        r += [em_row(i, e) for i, e in enumerate(self.emitters)]
        return r


class PrefabParticleFX(PrefabParticleBase):
    NODE = 'PREFAB_PARTICLE'

    def initialize(self):
        if self.env.count(self.tname) == 0:
            return
        if not FIX_prefab_exists(self.prefab):
            return
        if not self.init_prefab():
            return
        e = new_em(0)
        e['burst'] = 1 if self.bursts else 0
        self.emitters = [e]

    def on_event(self):
        if not self.init:
            return
        self.emitters[0]['playing'] = 1
        self.env.event(self, 'play', -1, 0, False, 0.0)

    def on_event_power(self, power):
        if not self.init:
            return
        e = self.emitters[0]
        if power <= 0.0:
            e['playing'] = 0
            return
        self.ep = self.emission.value(power)
        self.enp = self.energy.value(power)
        self.sp = self.speed.value(power)
        e['burst'] = 1 if self.bursts else 0
        e['resolved'] = 1
        e['ep'] = self.ep
        e['enp'] = self.enp
        e['sp'] = self.sp
        mn, mx, l0, l1, vel = self.base
        if self.bursts:
            e['minE'] = short(floor_int(prod(mn, self.ep)))
            e['maxE'] = short(floor_int(prod(mx, self.ep)))
        else:
            e['minE'] = floor_int(prod(mn, self.ep))
            e['maxE'] = e['minE']
        e['minEn'] = prod(l0, self.enp)
        e['maxEn'] = prod(l1, self.enp)
        e['vx'], e['vy'], e['vz'] = vel  # quirk: the speed curve writes local struct copies
        e['playing'] = 1


class PrefabMultiParticleFX(PrefabParticleBase):
    NODE = 'PREFAB_MULTI_PARTICLE'

    def initialize(self):
        n = self.env.count(self.tname)
        if n == 0:
            return
        if not FIX_prefab_exists(self.prefab):
            return
        if not self.init_prefab():
            return
        for _ in range(n):
            e = new_em(len(self.emitters))
            e['burst'] = 1 if self.bursts else 0
            self.emitters.append(e)

    def on_event_idx(self, idx):
        if not self.init:
            return
        if 0 <= idx < len(self.emitters):
            self.emitters[idx]['playing'] = 1
            self.env.event(self, 'play', idx, idx, False, 0.0)
            return
        for i in range(len(self.emitters)):
            self.emitters[i]['playing'] = 1
            self.env.event(self, 'play', idx, i, False, 0.0)

    def set_emitter(self, i, minv, maxv, minen, maxen, v):
        e = self.emitters[i]
        e['burst'] = 1 if self.bursts else 0
        if self.bursts:
            e['minE'] = short(minv)
            e['maxE'] = short(maxv)
        else:
            e['minE'] = minv
            e['maxE'] = minv
        e['minEn'] = minen
        e['maxEn'] = maxen
        e['vx'], e['vy'], e['vz'] = v
        e['resolved'] = 1
        e['ep'] = self.ep
        e['enp'] = self.enp
        e['sp'] = self.sp
        e['playing'] = 1

    def on_event_power_idx(self, power, idx):
        if not self.init:
            return
        if power <= 0.0:
            if 0 <= idx < len(self.emitters):
                self.emitters[idx]['playing'] = 0
                return
            for e in self.emitters:
                e['playing'] = 0
            return
        self.ep = self.emission.value(power)
        self.enp = self.energy.value(power)
        self.sp = self.speed.value(power)
        mn, mx, l0, l1, vel = self.base
        minv = floor_int(prod(mn, self.ep))
        maxv = floor_int(prod(mx, self.ep))
        minen = prod(l0, self.enp)
        maxen = prod(l1, self.enp)
        v = (prod(vel[0], self.sp), prod(vel[1], self.sp), prod(vel[2], self.sp))
        if 0 <= idx < len(self.emitters):
            self.set_emitter(idx, minv, maxv, minen, maxen, v)
            return
        for j in range(len(self.emitters)):
            self.set_emitter(j, minv, maxv, minen, maxen, v)


class AnimBase(Beh):
    def __init__(self):
        super().__init__()
        self.clip = ''
        self.tname = ''
        self.atype = 0
        self.has_state = False
        self.ntime = 0.0
        self.playing = False

    def fire(self, power):
        if self.atype == 1:
            self.ntime = power
            self.playing = True
            self.env.event(self, 'animPlay', -1, -1, True, power)
        else:
            self.ntime = power
            self.env.event(self, 'animSample', -1, -1, True, power)

    def base_rows(self):
        return [st('clipName', self.clip), st('transformName', self.tname), fl('animationType', float(self.atype))]

    def tail_rows(self):
        return [fl('found', 1.0 if self.has_state else 0.0), fl('normalizedTime', self.ntime), fl('playing', 1.0 if self.playing else 0.0)]


class AnimationFX(AnimBase):
    NODE = 'ANIMATION'

    def load(self, node):
        self.clip = p_str(node, 'clipName', self.clip)
        self.tname = p_str(node, 'transformName', self.tname)
        self.atype = p_enum(node, 'animationType', self.atype, ANIMTYPES)

    def save(self):
        out = [('clipName', self.clip), ('transformName', self.tname), ('animationType', ANIMTYPES[self.atype])]
        return out + out  # CopyTo onto itself doubles the values

    def find(self):
        self.has_state = FIX_part_anim(self.env.part, self.tname, self.clip)
        return self.has_state

    def initialize(self):
        self.find()

    def on_event(self):
        if not self.has_state:
            return
        self.ntime = 0.0
        self.fire(0.0)

    def on_event_power(self, power):
        if self.has_state or self.find():
            self.fire(power)

    def rows(self):
        return self.base_rows() + self.tail_rows()


class ModelAnimationFX(AnimBase):
    NODE = 'MODEL_ANIMATION'

    def __init__(self):
        super().__init__()
        self.model = ''
        self.anim = ''

    def load(self, node):
        self.model = p_str(node, 'modelName', self.model)
        self.tname = p_str(node, 'transformName', self.tname)
        self.anim = p_str(node, 'animationName', self.anim)
        self.clip = p_str(node, 'clipName', self.clip)
        self.atype = p_enum(node, 'animationType', self.atype, ANIMTYPES)

    def save(self):
        out = [('modelName', self.model), ('transformName', self.tname), ('animationName', self.anim), ('clipName', self.clip),
               ('animationType', ANIMTYPES[self.atype])]
        return out + out

    def initialize(self):
        if self.env.count(self.tname) == 0:
            return
        found = FIX_model(self.model)[0]
        if not found:
            return
        if not FIX_anim_found(self.model, self.anim, self.clip):
            return
        self.has_state = True

    def on_event(self):
        if not self.has_state:
            raise RuntimeError('NullReferenceException')
        self.fire(0.0)

    def on_event_power(self, power):
        if not self.has_state:
            raise RuntimeError('NullReferenceException')
        self.fire(power)

    def rows(self):
        return self.base_rows() + [st('modelName', self.model), st('animationName', self.anim)] + self.tail_rows()


class PrefabAnimationFX(AnimBase):
    NODE = 'PREFAB_ANIMATION'

    def __init__(self):
        super().__init__()
        self.prefab = ''
        self.anim = ''

    def load(self, node):
        self.prefab = p_str(node, 'prefabName', self.prefab)
        self.tname = p_str(node, 'transformName', self.tname)
        self.anim = p_str(node, 'animationName', self.anim)
        self.clip = p_str(node, 'clipName', self.clip)
        self.atype = p_enum(node, 'animationType', self.atype, ANIMTYPES)

    def save(self):
        out = [('prefabName', self.prefab), ('transformName', self.tname), ('animationName', self.anim), ('clipName', self.clip),
               ('animationType', ANIMTYPES[self.atype])]
        return out + out

    def initialize(self):
        if self.env.count(self.tname) == 0:
            return
        if not FIX_prefab_exists(self.prefab):
            return
        if not FIX_anim_found(self.prefab, self.anim, self.clip):
            return
        self.has_state = True

    def on_event(self):
        if not self.has_state:
            raise RuntimeError('NullReferenceException')
        self.fire(0.0)

    def on_event_power(self, power):
        if not self.has_state:
            raise RuntimeError('NullReferenceException')
        self.fire(power)

    def rows(self):
        return self.base_rows() + [st('prefabName', self.prefab), st('animationName', self.anim)] + self.tail_rows()


class PrefabSpawnFX(Beh):
    NODE = 'PREFAB_SPAWN'

    def __init__(self):
        super().__init__()
        self.prefab = ''
        self.tname = ''
        self.set_parent = True
        self.has_parent = False
        self.has_prefab = False
        self.spawned = 0

    def load(self, node):
        self.prefab = p_str(node, 'prefabName', self.prefab)
        self.tname = p_str(node, 'transformName', self.tname)
        self.set_parent = p_bool(node, 'setParent', self.set_parent)

    def save(self):
        return [('prefabName', self.prefab), ('transformName', self.tname), ('setParent', 'True' if self.set_parent else 'False')]

    def initialize(self):
        self.has_parent = self.env.count(self.tname) > 0
        if not self.has_parent:
            return
        self.has_prefab = FIX_prefab_exists(self.prefab)

    def spawn(self):
        if self.has_parent and self.has_prefab:
            self.spawned += 1
            self.env.event(self, 'spawn', -1, -1, False, 0.0)

    def on_event(self):
        self.spawn()

    def on_event_power(self, power):
        self.spawn()

    def rows(self):
        return [st('prefabName', self.prefab), st('transformName', self.tname), fl('setParent', 1.0 if self.set_parent else 0.0), fl('spawned', float(self.spawned))]


class ParticleConfigFX(Beh):
    NODE = 'PARTICLE_CONFIG'

    def __init__(self):
        super().__init__()
        self.power = 0.0
        self.one_shot = False
        self.min_size = FXCurve(0.1)
        self.max_size = FXCurve(0.1)
        self.min_energy = FXCurve(2.0)
        self.max_energy = FXCurve(3.0)
        self.min_emission = FXCurve(80.0)
        self.max_emission = FXCurve(100.0)
        self.size_grow = FXCurve(0.0)
        self.length_scale = FXCurve(0.0)
        self.transform = ''
        self.init = False
        self.v = [0.0] * 6

    def load(self, node):
        self.power = p_float(node, 'power', self.power)
        self.one_shot = p_bool(node, 'oneShot', self.one_shot)
        t = node.get('transform')
        if t is not None:
            self.transform = t
        self.min_size.load('minSize', node)
        self.max_size.load('maxSize', node)
        self.min_energy.load('minEnergy', node)
        self.max_energy.load('maxEnergy', node)
        self.min_emission.load('minEmission', node)
        self.max_emission.load('maxEmission', node)

    def save(self):
        return []

    def initialize(self):
        self.power = 0.0
        self.init = True
        self.update_components()

    def update_components(self):
        p = self.power
        self.v = [self.min_size.value(p), self.max_size.value(p), self.min_energy.value(p), self.max_energy.value(p),
                  f32((1.0 + self.size_grow.value(p)) * self.max_energy.value(p)), self.length_scale.value(p)]

    def on_event_power(self, power):
        if not self.one_shot:
            self.update_components()

    def on_event(self):
        if self.one_shot:
            self.power = 1.0
            self.update_components()
            # Random.Range(minEmission, maxEmission): the draw is the global generator's (not modelled): the event carries the FXCurve bounds
            self.env.event(self, 'emit', -1, 0, True, self.min_emission.last)

    def rows(self):
        n = ['startSizeMin', 'startSizeMax', 'startLifetimeMin', 'startLifetimeMax', 'sizeOverLifetimeEnd', 'lengthScale']
        r = [st('transform', self.transform), fl('power', self.power), fl('oneShot', 1.0 if self.one_shot else 0.0), fl('initialized', 1.0 if self.init else 0.0)]
        r += [fl(a, b) for a, b in zip(n, self.v)]
        return r


class ROCsSFX(Beh):
    NODE = 'ROCSAUDIO'

    def load(self, node):
        pass

    def save(self):
        return []

    def rows(self):
        return []


CLASSES = {c.NODE: c for c in (AudioFX, AudioLoopFX, AudioSingleFX, AudioMultiFX, AudioMultiPooledFX, ModelParticleFX, ModelMultiParticleFX, PrefabParticleFX,
                               PrefabMultiParticleFX, AnimationFX, ModelAnimationFX, PrefabAnimationFX, PrefabSpawnFX, ParticleConfigFX, ROCsSFX)}


# ------------------------------------------------------------------------------------------------------------------------------
# EffectList


class EffectList:
    def __init__(self, env):
        self.env = env
        self.entries = []  # [name, [beh]]
        self.errors = []

    def find(self, name):
        for e in self.entries:
            if e[0] == name:
                return e[1]
        return None

    def on_load(self, node):
        for n in node.nodes:
            self.load_effect(n)

    def load_effect(self, node):
        name = node.name
        if self.find(name) is None:
            self.entries.append([name, []])
        for cn in node.nodes:
            cls = CLASSES.get(cn.name)
            if cls is None:
                self.errors.append(cn.name)
                continue
            inst = cn.get('name')
            b = None
            for x in self.find(name):
                if (inst is None or x.instance_name == inst) and x.NODE == cls.NODE:
                    b = x
                    break
            if b is None:
                b = cls()
                b.env = self.env
                b.effect_name = name
                b.instance_name = inst
                self.find(name).append(b)
            b.load(cn)

    def prune(self):
        self.entries = [e for e in self.entries if e[1]]

    def initialize(self):
        for _, lst in self.entries:
            for b in lst:
                b.initialize()

    def event(self, name, power, idx):
        lst = self.find(name)
        if lst is None:
            return
        for b in lst:
            if power is None:
                b.on_event_idx(idx)
            else:
                b.on_event_power_idx(power, idx)

    def update(self, dt):
        for _, lst in self.entries:
            for b in lst:
                b.update(dt)

    def pause(self):
        for _, lst in self.entries:
            for b in lst:
                if b.subscribed:
                    b.pause()

    def unpause(self):
        for _, lst in self.entries:
            for b in lst:
                if b.subscribed:
                    b.unpause()

    def save_rows(self):
        out = []
        for name, lst in self.entries:
            out.append('node %s' % name)
            for b in lst:
                out.append(' node %s' % b.NODE)
                if b.instance_name:
                    out.append('  v name=%s' % b.instance_name)
                for k, v in b.save():
                    out.append('  v %s=%s' % (k, v))
        return out

    def state_rows(self):
        rows = []
        order = {'f ': 0, 's ': 1, 'sr': 2, 'em': 3}
        for _, lst in self.entries:
            for b in lst:
                rows.append(b.inst_row())
                rows += sorted(b.rows(), key=lambda r: order[r[:2]])  # floats, strings, sources, emitters (stable)
        return rows

    def event_rows(self):
        rows = []
        for ev in self.env.events:
            rows.append('ev %s|%s|%s|%s|%d|%d|%d|%08x' % (ev[0], ev[1], ev[2], ev[3], ev[4], ev[5], ev[6], bits(ev[7])))
        self.env.events = []
        return rows


def build_list(env, effects_nodes):
    lst = EffectList(env)
    for n in effects_nodes:
        lst.on_load(n)
    lst.prune()
    return lst


# ------------------------------------------------------------------------------------------------------------------------------
# step scripts


def hx(x):
    return '%08x' % bits(x)


POWERS = [0.0, 0.03, 0.1, 0.25, 0.5, 0.75, 1.0, 1.4, -1.0]


def sweep_script(effect_names):
    steps = ['i']
    for name in effect_names:
        for p in POWERS:
            steps.append('e %s %s -1' % (name, hx(f32(p))))
            steps.append('u %s' % hx(f32(0.016)))
        for p, idx in ((0.6, 0), (0.6, 1), (0.0, 1), (0.4, 7), (0.9, 2)):
            steps.append('e %s %s %d' % (name, hx(f32(p)), idx))
            steps.append('u %s' % hx(f32(0.016)))
        steps.append('e %s - -1' % name)
        steps.append('e %s - 1' % name)
        steps.append('u %s' % hx(f32(0.5)))
        steps.append('u %s' % hx(f32(3.0)))
    if effect_names:
        steps.append('p')
        steps.append('e %s %s -1' % (effect_names[0], hx(f32(0.5))))
        steps.append('u %s' % hx(f32(0.016)))
        steps.append('r')
        steps.append('u %s' % hx(f32(0.016)))
        steps.append('e %s %s -1' % (effect_names[-1], hx(f32(0.0))))
    return steps


def early_script(effect_names):
    steps = []
    for name in effect_names:
        steps.append('e %s %s -1' % (name, hx(f32(0.5))))
        steps.append('e %s %s -1' % (name, hx(f32(0.0))))
        steps.append('e %s - -1' % name)
    steps.append('i')
    for name in effect_names:
        steps.append('e %s %s -1' % (name, hx(f32(0.5))))
        steps.append('e %s - -1' % name)
        steps.append('u %s' % hx(f32(0.016)))
    steps.append('i')  # a second OnInitialize (the multi emitters append)
    for name in effect_names:
        steps.append('e %s %s -1' % (name, hx(f32(1.0))))
    return steps


def run_steps(part, env, lst, steps):
    out = []
    digests = []
    h = 0xcbf29ce484222325
    for s in steps:
        t = s.split(' ')
        rows = []
        try:
            if t[0] == 'i':
                lst.initialize()
            elif t[0] == 'e':
                power = None if t[2] == '-' else struct.unpack('<f', struct.pack('<I', int(t[2], 16)))[0]
                lst.event(t[1], power, int(t[3]))
            elif t[0] == 'u':
                lst.update(struct.unpack('<f', struct.pack('<I', int(t[1], 16)))[0])
            elif t[0] == 'p':
                lst.pause()
            elif t[0] == 'r':
                lst.unpause()
            elif t[0] == 'x':  # exceptions of a step are part of the state
                pass
        except (IndexError, RuntimeError) as ex:
            rows.append('exc %s' % type(ex).__name__)
        rows.append('step %s' % s)
        rows += lst.state_rows()
        rows += lst.event_rows()
        for r in rows:
            h = fnv64(r.encode('utf-8') + b'\n', h)
            ROWS[0] += 1
        digests.append(h & 0xFFFFFFFF)
        out.append(rows)
    return h, digests, out


# ------------------------------------------------------------------------------------------------------------------------------
# parts


def collect_parts(mirror):
    gd = os.path.join(mirror, 'GameData')
    res = []
    for d, _, fs in os.walk(gd):
        for f in sorted(fs):
            if not f.endswith('.cfg'):
                continue
            full = os.path.join(d, f)
            rel = os.path.relpath(full, gd).replace('\\', '/')
            try:
                text = open(full, encoding='utf-8-sig', errors='replace').read()
            except OSError:
                continue
            if 'EFFECTS' not in text and 'fx_' not in text and 'sound_' not in text:
                continue
            root = parse_cfg(text)
            pidx = 0
            for pn in root.nodes:
                if pn.name != 'PART':
                    continue
                effs = [n for n in pn.nodes if n.name == 'EFFECTS']
                res.append((rel if pidx == 0 else '%s#%d' % (rel, pidx), pn, effs))
                pidx += 1
    res.sort(key=lambda x: x[0])
    return res


FIX_FILE_HEADER = 'labelled fixture'


FIXTURE_TEXT = '''
EFFECTS
{
	fxFixture
	{
		AUDIO_SINGLE
		{
			name = single
			channel = UI
			clip = fixture/sound_single
			polyphony = 2
			volume = 0.0 0.0
			volume = 1.0 1.0
			pitch = 0.5
		}
		AUDIO_SINGLE
		{
			name = single2
			clip = fixture/sound_single2.wav
		}
		AUDIO_MULTI
		{
			channel = Ambient
			clip = fixture/sound_multi
			transformName = thrustTransform
			volume = 0.0 0.0
			volume = 0.5 0.5
			volume = 1.0 1.0
			pitch = 1.0 1.0
			pitch = 0.0 0.5
		}
		PREFAB_MULTI_PARTICLE
		{
			prefabName = fixture/prefab_multi
			transformName = thrustTransform
			emission = 0.0 0.0
			emission = 1.0 100.0
			energy = 0.0 0.5
			energy = 1.0 1.5
			speed = 1.2
			localOffset = 0, 0.5, 0
			localRotation = 0, 0, 1, 90
		}
		ANIMATION
		{
			clipName = deploy
			transformName = thrustTransform
			animationType = Play
		}
		MODEL_ANIMATION
		{
			modelName = fixture/anim_model
			transformName = thrustTransform
			animationName = anim
			clipName = clip
			animationType = HoldValue
		}
		PREFAB_ANIMATION
		{
			prefabName = fixture/anim_prefab
			transformName = thrustTransform
			animationName = anim
			clipName = clip
			animationType = Play
		}
		PREFAB_SPAWN
		{
			prefabName = fixture/spawn
			transformName = thrustTransform
			setParent = false
		}
		PARTICLE_CONFIG
		{
			power = 0.25
			oneShot = false
			transform = thrustTransform
			minSize = 0.0 0.1
			minSize = 1.0 0.5
			maxSize = 0.3
			minEnergy = 1.5
			maxEnergy = 2.5
			minEmission = 40
			maxEmission = 60
		}
		PARTICLE_CONFIG
		{
			name = oneshot
			oneShot = true
			minEmission = 10
			maxEmission = 20
		}
		ROCSAUDIO
		{
			name = roc
		}
		UNKNOWN_CLASS
		{
			name = nothing
		}
	}
}
'''


def fixture_parts():
    """EFFECTS nodes for the cfg forms no stock part uses (test fixtures, not part definitions)."""
    root = parse_cfg(FIXTURE_TEXT)
    pn = Node('PART')
    pn.values.append(('name', 'fixtureEffects'))
    m1 = Node('MODULE')
    m1.values.append(('name', 'ModuleEnginesFX'))
    m2 = Node('MODULE')
    m2.values.append(('name', 'ModuleRCSFX'))
    pn.nodes += [root.nodes[0], m1, m2]
    return [('fixture/EFFECTS-all-classes.cfg', pn, [root.nodes[0]])]


# ------------------------------------------------------------------------------------------------------------------------------
# module call models (ModuleEnginesFX, ModuleRCSFX)


def fx_lerp(a, b, t):
    t = 0.0 if t < 0.0 else (1.0 if t > 1.0 else t)
    return f32(a + (b - a) * t)


class EngineModel:
    """The part.Effect calls of ModuleEnginesFX (ModuleEnginesFX.cs, ModuleEngines.cs Activate / Shutdown / Flameout / UnFlameout)."""

    DEFAULT = dict(flameout='flameout', running='running', power='power', engage='engage', disengage='disengage', directThrottle='directThrottle', spool='spool')

    def __init__(self, lst, names, use_resp=True, spool_time=f32(2.0), spool_idle=f32(0.05), max_thrust=f32(120.0), allow_shutdown=True, allow_restart=True):
        self.allow_shutdown = allow_shutdown
        self.allow_restart = allow_restart
        self.engine_shutdown = False
        self.names = names
        self.lst = lst
        self.calls = []
        self.ignited = False
        self.flameout = False
        self.current = 0.0
        self.requested = 0.0
        self.final = 0.0
        self.max_thrust = max_thrust
        self.use_resp = use_resp
        self.spool_time = spool_time
        self.spool_idle = spool_idle
        self.spool = 0.0

    def eff(self, key, power=None, idx=-1):
        name = self.names[key]
        self.calls.append((name, power, idx))
        self.lst.event(name, power, idx)

    def deactivate_power(self):
        self.eff('power', 0.0)

    def deactivate_running(self):
        self.eff('running', 0.0)

    def activate(self):  # ModuleEngines.Activate :740 (shielded check passes)
        if not self.allow_restart and self.engine_shutdown:
            return
        if not self.ignited:
            self.eff('engage')
        self.ignited = True

    def shutdown(self):  # ModuleEngines.Shutdown :788
        if not self.allow_shutdown:
            return
        if not self.allow_restart:
            self.engine_shutdown = True
        self.ignited = False
        self.eff('disengage')
        self.deactivate_power()
        self.deactivate_running()

    def do_flameout(self, status_only=False, show=True):  # Flameout :497
        if not status_only:
            if not self.flameout and show:
                self.eff('flameout')
            self.flameout = True
            if not self.allow_restart:
                self.shutdown()
        self.current = 0.0

    def unflameout(self, show=True):  # UnFlameout :517 (ActivateRunningFX is a no-op without groups)
        if self.flameout and show:
            self.eff('flameout')
        self.flameout = False

    def fx_update(self, dt):  # FXUpdate :59
        if self.ignited:
            self.eff('directThrottle', self.requested)
        else:
            self.eff('directThrottle', 0.0)
        if self.ignited and not self.flameout:
            if self.use_resp:
                self.spool = fx_lerp(self.spool, self.spool_idle if self.spool_idle > self.current else self.current, f32(self.spool_time * dt))
            else:
                self.spool = self.current
            self.eff('spool', self.spool)
            self.eff('running', self.current)
            if self.final == 0.0:
                self.eff('power', 0.0)
            else:
                self.eff('power', f32(self.final / self.max_thrust))
        else:
            if self.use_resp:
                self.spool = fx_lerp(self.spool, 0.0 if self.flameout else self.current, f32(self.spool_time * dt))
            else:
                self.spool = 0.0
            self.eff('spool', self.spool)
            self.eff('running', 0.0)
            self.eff('power', 0.0)


def engine_script(names_present):
    """Scripted engine sequence as step strings: ignite, throttle up / down, flameout, restart, shutdown (the module state set per step:
    currentThrottle, requestedThrottle, finalThrust, deltaTime as float bit patterns)."""
    z = hx(0.0)
    dt = hx(f32(0.02))
    s = ['m activate', 'm set %s %s %s %s' % (z, z, z, dt)]
    for k in range(0, 21):
        th = f32(k / 20.0)
        s.append('m set %s %s %s %s' % (hx(th), hx(th), hx(f32(th * 120.0)), dt))
        s.append('u %s' % hx(f32(0.02)))
    for k in range(20, -1, -4):
        th = f32(k / 20.0)
        s.append('m set %s %s %s %s' % (hx(th), hx(th), hx(f32(th * 120.0)), dt))
    h5 = hx(f32(0.5))
    h3 = hx(f32(0.3))
    s += ['m flameout', 'm set %s %s %s %s' % (z, z, z, dt), 'm unflameout', 'm set %s %s %s %s' % (h5, h5, h5, dt)]
    s += ['m shutdown', 'm set %s %s %s %s' % (z, z, z, dt), 'm activate', 'm set %s %s %s %s' % (h3, h3, hx(f32(36.0)), dt), 'm flameout statusonly',
          'm set %s %s %s %s' % (h3, h3, z, dt)]
    return s


def hexbits_to_float(h):
    return struct.unpack('<f', struct.pack('<I', int(h, 16)))[0]


def module_params(pnode, cls):
    for n in pnode.nodes:
        if n.name == 'MODULE' and n.get('name') == cls:
            return n
    return None


def engine_names(mod):
    d = dict(EngineModel.DEFAULT)
    if mod is not None:
        for key, field in (('flameout', 'flameoutEffectName'), ('running', 'runningEffectName'), ('power', 'powerEffectName'), ('engage', 'engageEffectName'),
                           ('disengage', 'disengageEffectName'), ('directThrottle', 'directThrottleEffectName'), ('spool', 'spoolEffectName')):
            d[key] = p_str(mod, field, d[key])
    return d


def awake_all(pnode, lst):
    """Part.Awake of the instance: every module's OnAwake part.Effect calls, in module order (ModuleEnginesFX.cs:162, ModuleRCSFX.cs:30)."""
    for n in pnode.nodes:
        if n.name != 'MODULE':
            continue
        cls = n.get('name')
        if cls == 'ModuleEnginesFX':
            names = engine_names(n)
            for k in ('flameout', 'running', 'power', 'engage', 'disengage', 'directThrottle', 'spool'):
                lst.event(names[k], 0.0, -1)
        elif cls == 'ModuleRCSFX':
            lst.event(p_str(n, 'runningEffectName', 'running'), 0.0, -1)


def run_engine(env, lst, mode_resp, steps, mod):
    names = engine_names(mod)
    eng = EngineModel(lst, names, use_resp=mode_resp, spool_time=p_float(mod, 'engineSpoolTime', f32(2.0)) if mod else f32(2.0),
                      spool_idle=p_float(mod, 'engineSpoolIdle', f32(0.05)) if mod else f32(0.05), max_thrust=p_float(mod, 'maxThrust', f32(215.0)) if mod else f32(120.0),
                      allow_shutdown=p_bool(mod, 'allowShutdown', True) if mod else True, allow_restart=p_bool(mod, 'allowRestart', True) if mod else True)
    digests = []
    h = 0xcbf29ce484222325
    for s in steps:
        t = s.split(' ')
        rows = []
        if t[0] == 'm':
            if t[1] == 'activate':
                eng.activate()
            elif t[1] == 'shutdown':
                eng.shutdown()
            elif t[1] == 'flameout':
                eng.do_flameout(status_only=len(t) > 2)
            elif t[1] == 'unflameout':
                eng.unflameout()
            elif t[1] == 'set':
                eng.current = hexbits_to_float(t[2])
                eng.requested = hexbits_to_float(t[3])
                eng.final = hexbits_to_float(t[4])
                eng.fx_update(hexbits_to_float(t[5]))
        elif t[0] == 'u':
            lst.update(hexbits_to_float(t[1]))
        for c in eng.calls:
            rows.append('call %s|%s|%d' % (c[0], '-' if c[1] is None else hx(c[1]), c[2]))
        eng.calls = []
        rows.append('step %s' % s)
        rows += lst.state_rows()
        rows += lst.event_rows()
        for r in rows:
            h = fnv64(r.encode('utf-8') + b'\n', h)
            ROWS[0] += 1
        digests.append(h & 0xFFFFFFFF)
    return h, digests


def rcs_script():
    s = []
    dt = hx(f32(0.02))
    for k in range(6):
        s.append('m up %d %s %s' % (k % 3, hx(f32(0.1 + 0.18 * k)), '1'))
        s.append('u %s' % dt)
    s += ['m up 0 %s 0' % hx(f32(0.5)), 'm deact', 'm up 2 %s 1' % hx(f32(1.0)), 'm deactpower']
    return s


def run_rcs(env, lst, steps, mod):
    rname = p_str(mod, 'runningEffectName', 'running') if mod is not None else 'running'
    digests = []
    h = 0xcbf29ce484222325
    for s in steps:
        t = s.split(' ')
        rows = []
        calls = []
        if t[0] == 'm':
            if t[1] == 'up':
                running = t[4] == '1'
                power = hexbits_to_float(t[3])
                calls.append((rname, power if running else 0.0, int(t[2])))
            elif t[1] in ('deact', 'deactpower'):
                calls.append((rname, 0.0, -1))
        elif t[0] == 'u':
            lst.update(hexbits_to_float(t[1]))
        for c in calls:
            lst.event(c[0], c[1], c[2])
            rows.append('call %s|%s|%d' % (c[0], hx(c[1]), c[2]))
        rows.append('step %s' % s)
        rows += lst.state_rows()
        rows += lst.event_rows()
        for r in rows:
            h = fnv64(r.encode('utf-8') + b'\n', h)
            ROWS[0] += 1
        digests.append(h & 0xFFFFFFFF)
    return h, digests


# ------------------------------------------------------------------------------------------------------------------------------
# legacy FXGroup


class Light:
    def __init__(self):
        self.enabled = True
        self.intensity = 1.0


class PS:
    def __init__(self, a, b, c, d):
        self.playing = False
        self.smin, self.smax, self.lmin, self.lmax = a, b, c, d
        self.two = False
        self.plays = 0


def lmax(a, b):
    return a if a > b else b


class FXGroup:
    def __init__(self, name):
        self.name = name
        self.ps = []
        self.lights = []
        self.sfx = ''
        self.audio = None
        self.valid = False
        self.active = False
        self.power = 0.0
        self.pvar = 0.0
        self.clamp = False
        self.minv = 0.0
        self.maxv = 1.0
        self.latch = False
        self.oneshots = 0
        self.fade = None
        self.now = 0.0

    def begin(self, audio_ref):
        self.valid = True
        self.il = [l.intensity for l in self.lights]
        for l in self.lights:
            l.enabled = False
        self.isz = []
        self.ilf = []
        self.iszv = []
        self.ilfv = []
        for p in self.ps:
            self.isz.append(lmax(p.smax, p.smin))
            self.ilf.append(lmax(p.lmax, p.lmin))
            self.iszv.append(abs(f32(p.smax - p.smin)))
            self.ilfv.append(abs(f32(p.lmax - p.lmin)))
            p.playing = False
        if self.audio is None:
            self.audio = audio_ref
        if self.sfx and self.audio is not None:
            self.audio.clip = self.sfx
        self.active = False

    def set_active(self, value):
        if not self.valid:
            return
        for l in self.lights:
            l.enabled = value
        for p in self.ps:
            p.playing = value
            if value:
                p.plays += 1
        if self.sfx and self.audio is not None:
            self.audio.clip = self.sfx
            if value:
                if not self.audio.playing:
                    self.audio.play()
                    self.audio.elapsed = 0.0  # Random.Range(0, length) is not modelled (the generator is global): the test seeds it identically
            else:
                self.audio.stop()
        self.active = value

    def set_power(self, pwr):
        if self.clamp:
            self.power = fx_lerp(self.minv, self.maxv, pwr)
        else:
            self.power = pwr
        if not self.valid:
            return
        for i, l in enumerate(self.lights):
            l.intensity = f32(self.il[i] * pwr)
        for j, p in enumerate(self.ps):
            p.lmin = f32((self.ilf[j] - self.ilfv[j]) * self.power)
            p.lmax = f32(self.ilf[j] * self.power)
            p.smin = f32((self.isz[j] - self.iszv[j]) * self.power)
            p.smax = f32(self.isz[j] * self.power)
            p.two = True
        if self.sfx and self.audio is not None:
            self.audio.pitch = pwr

    def set_power_latch(self, pwr):
        if not self.latch:
            return
        if self.clamp:
            self.power = fx_lerp(self.minv, self.maxv, pwr)
        else:
            self.power = 0.0 if pwr < 0 else (1.0 if pwr > 1.0 else pwr)
        if not self.valid:
            return
        for i, l in enumerate(self.lights):
            l.intensity = lmax(f32(self.il[i] * pwr), l.intensity)
        for j, p in enumerate(self.ps):
            p.lmin = lmax(f32(self.ilf[j] * self.power - self.pvar * 0.5), p.lmin)
            p.lmax = lmax(f32(self.ilf[j] * self.power + self.pvar * 0.5), p.lmax)
            p.smin = lmax(f32(self.isz[j] * self.power - self.pvar * 0.5), p.smin)
            p.smax = lmax(f32(self.isz[j] * self.power + self.pvar * 0.5), p.smax)
            p.two = True
        if self.sfx and self.audio is not None:
            self.audio.pitch = lmax(pwr, self.audio.pitch)

    def burst(self):
        if not self.valid:
            return
        for p in self.ps:
            p.playing = True
            p.plays += 1
        if self.sfx and self.audio is not None:
            self.oneshots += 1

    def unlatch(self):
        if self.valid and self.latch:
            self.set_active(False)
            self.latch = False
            self.set_power(self.power)

    def row(self):
        r = ['grp %s|%d|%d|%d|%d|%08x|%08x|%08x|%08x|%s|%d|%d' % (self.name, self.valid, self.active, self.latch, self.clamp, bits(self.power), bits(self.pvar),
                                                          bits(self.minv), bits(self.maxv), self.sfx, 1 if self.audio is not None else 0, self.oneshots)]
        for p in self.ps:
            r.append(' ps %d|%d|%08x|%08x|%08x|%08x|%d' % (p.playing, p.plays, bits(p.smin), bits(p.smax), bits(p.lmin), bits(p.lmax), p.two))
        for l in self.lights:
            r.append(' li %d|%08x' % (l.enabled, bits(l.intensity)))
        if self.audio is not None:
            r.append(' ' + self.audio.row())
        return r


def build_groups(part_node):
    """Part.OnLoad adds the four default groups; the fx_* lines (PartLoader.cs:826) and sound_* lines (:1507) populate them."""
    names = ['prelaunch', 'activate', 'active', 'deactivate']
    groups = {n: FXGroup(n) for n in names}
    order = list(names)
    for k, v in part_node.values:
        if k.startswith('fx'):
            a = v.split(',')
            if len(a) < 7:
                continue
            for j in range(6, len(a)):
                t = a[j].strip()
                if t == 'true':
                    continue
                if t not in groups:
                    groups[t] = FXGroup(t)
                    order.append(t)
                h = fnv_str(k)
                groups[t].ps.append(PS(f32(0.1 + (h % 5) * 0.1), f32(0.5 + (h % 7) * 0.25), f32(0.5 + ((h >> 8) % 5) * 0.5), f32(1.0 + ((h >> 12) % 5) * 0.5)))
    for k, v in part_node.values:
        if k.startswith('sound'):
            if not FIX_clip_exists(k):
                continue
            for text in v.split(','):
                t = text.strip()
                if t not in groups:
                    groups[text] = FXGroup(text)
                    order.append(text)
                    groups[text].sfx = k
                else:
                    groups[t].sfx = k
    return groups, order


def group_script(order):
    s = ['b']
    for n in order:
        s += ['g %s a 1' % n, 'g %s p %s' % (n, hx(f32(0.6))), 'g %s p %s' % (n, hx(f32(1.7))), 'g %s burst' % n, 'g %s a 0' % n,
              'g %s mm %s %s' % (n, hx(f32(0.2)), hx(f32(0.9))), 'g %s p %s' % (n, hx(f32(0.5))), 'g %s latch' % n, 'g %s pl %s' % (n, hx(f32(0.8))),
              'g %s var %s' % (n, hx(f32(0.3))), 'g %s pl %s' % (n, hx(f32(0.9))), 'g %s unlatch' % n]
    return s


def run_groups(part_audio, groups, order, steps):
    digests = []
    h = 0xcbf29ce484222325
    for s in steps:
        t = s.split(' ')
        if t[0] == 'b':
            for n in reversed(order):
                groups[n].begin(part_audio)
        elif t[0] == 'g':
            g = groups[t[1]]
            op = t[2]
            if op == 'a':
                g.set_active(t[3] == '1')
            elif op == 'p':
                g.set_power(hexbits_to_float(t[3]))
            elif op == 'pl':
                g.set_power_latch(hexbits_to_float(t[3]))
            elif op == 'burst':
                g.burst()
            elif op == 'mm':
                g.minv = hexbits_to_float(t[3])
                g.maxv = hexbits_to_float(t[4])
                g.clamp = True
            elif op == 'latch':
                if g.valid:
                    g.set_active(True)
                    g.latch = True
            elif op == 'var':
                g.pvar = hexbits_to_float(t[3])
            elif op == 'unlatch':
                g.unlatch()
        rows = ['step %s' % s]
        for n in order:
            rows += groups[n].row()
        for r in rows:
            h = fnv64(r.encode('utf-8') + b'\n', h)
            ROWS[0] += 1
        digests.append(h & 0xFFFFFFFF)
    return h, digests


# ------------------------------------------------------------------------------------------------------------------------------


# ------------------------------------------------------------------------------------------------------------------------------
# FXModule* models (FXModuleAnimateThrottle.cs, FXModuleAnimateRCS.cs, FXModuleLookAtConstraint.cs, FXModuleConstrainPosition.cs)


def clamp01(x):
    return 0.0 if x < 0.0 else (1.0 if x > 1.0 else x)


class AnimThrottle:
    def __init__(self, mod, eng):
        self.mod = mod
        self.eng = eng  # engine module node
        self.rs = p_float(mod, 'responseSpeed', f32(0.5))
        self.dep_state = p_bool(mod, 'dependOnEngineState', False)
        self.dep_output = p_bool(mod, 'dependOnOutput', False)
        self.dep_throttle = p_bool(mod, 'dependOnThrottle', False)
        self.weight_op = p_bool(mod, 'weightOnOperational', False)
        self.base_speed = p_float(mod, 'baseAnimSpeed', 0.0)
        self.wrap = p_int(mod, 'animWrapMode', 8)
        self.affect_time = p_bool(mod, 'affectTime', True)
        self.speed_mult = p_float(mod, 'baseAnimSpeedMult', f32(1.0))
        self.play_editor = p_bool(mod, 'playInEditor', False)
        self.layer = p_int(mod, 'layer', 1)
        self.anim_state = p_float(mod, 'animState', 0.0)
        self.enabled = True
        self.weight = 0.0
        self.speed = 1.0
        self.ntime = 0.0
        self.playing = False
        self.max_thrust = p_float(eng, 'maxThrust', f32(215.0))

    def start(self):
        self.weight = 1.0
        self.speed = self.base_speed
        self.ntime = 0.0

    def step(self, scene, vessel, throttle, ign, flame, cur, fin):
        editor = scene == 'E'
        flight = scene == 'F'
        if (not editor and not flight) or not vessel:
            return
        if not self.playing:
            self.playing = True
        dt = f32(0.02)
        operational = (not flame) and ign
        norm_out = f32(fin / self.max_thrust)
        num = 0.0
        if not self.dep_state:
            num = (1.0 if self.play_editor else 0.0) if editor else clamp01(throttle)
        else:
            if self.weight_op:
                num = (1.0 if operational else 0.0) if (not editor or not self.play_editor) else 1.0
                num2 = fx_lerp(self.weight, num, f32(self.rs * 3.0 * 25.0 * dt))
                if num2 > f32(0.999) and self.weight < num:
                    num2 = 1.0
                elif num2 < f32(0.001) and num < self.weight:
                    num2 = 0.0
                self.weight = num2
            if operational and not editor:
                num = clamp01(norm_out) if self.dep_output else (clamp01(throttle) if not self.dep_throttle else clamp01(cur))
            else:
                num = 0.0 if (not editor or not self.play_editor) else 1.0
        if self.affect_time:
            num2 = fx_lerp(self.anim_state, num, f32(self.rs * 25.0 * dt))
            if num2 > f32(0.99995) and self.anim_state < num:
                num2 = 1.0
            if num2 < f32(5e-05) and num < self.anim_state:
                num2 = 0.0
            self.ntime = num2
        else:
            num3 = 0.0
            if num > 0.0 or (self.dep_state and operational):
                num3 = self.base_speed
            num2 = fx_lerp(self.anim_state, f32(num3 + num * self.speed_mult), f32(self.rs * dt))
            if num2 > f32(0.99995):
                num2 = 1.0
            if num2 < f32(5e-05):
                num2 = 0.0
            self.speed = num2
        if not editor:
            self.anim_state = num2

    def row(self):
        return 'am %d|%d|%08x|%08x|%08x|%d|%d|%d|%08x' % (1 if self.enabled else 0, 1 if self.dep_state else 0, bits(self.weight), bits(self.speed), bits(self.ntime),
                                                    self.layer, self.wrap, 1 if self.playing else 0, bits(self.anim_state))


class AnimRCS:
    def __init__(self, mod, part):
        self.rs = p_float(mod, 'responseSpeed', f32(0.5))
        self.base_speed = p_float(mod, 'baseAnimSpeed', 0.0)
        self.affect_time = p_bool(mod, 'affectTime', True)
        self.speed_mult = p_float(mod, 'baseAnimSpeedMult', f32(1.0))
        self.force_mult = p_float(mod, 'thrustForceMult', f32(1.0))
        h = fnv_str(part)
        self.n = 2 + h % 2
        self.transforms = self.n + (1 if h % 5 == 0 else 0)
        self.enabled = self.n == self.transforms
        self.state = [0.0] * self.n
        self.ntime = [0.0] * self.n
        self.speed = [self.base_speed] * self.n
        self.playing = [False] * self.n

    def step(self, scene, forces):
        editor = scene == 'E'
        if not self.enabled:
            return
        if not editor and scene != 'F':
            return
        dt = f32(0.02)
        for i in range(self.n):
            self.playing[i] = True
            num = clamp01(f32(forces[i] * self.force_mult))
            if self.affect_time:
                num2 = fx_lerp(self.state[i], num, f32(self.rs * 25.0 * dt))
                if num2 > f32(0.99995) and self.state[i] < num:
                    num2 = 1.0
                if num2 < f32(5e-05) and num < self.state[i]:
                    num2 = 0.0
                self.ntime[i] = num2
            else:
                num3 = 0.0
                if num > 0.0:
                    num3 = self.base_speed
                num2 = fx_lerp(self.state[i], f32(num3 + num * self.speed_mult), f32(self.rs * dt))
                if num2 > f32(0.99995):
                    num2 = 1.0
                if num2 < f32(5e-05):
                    num2 = 0.0
                self.speed[i] = num2
            if not editor:
                self.state[i] = num2

    def rows(self):
        r = ['ar %d' % (1 if self.enabled else 0)]
        for i in range(self.n):
            r.append(' a %08x|%08x|%08x|%d' % (bits(self.state[i]), bits(self.ntime[i]), bits(self.speed[i]), 1 if self.playing[i] else 0))
        return r

    def save(self):
        return ','.join(g7(x) for x in self.state)


def hash_rows(rows, h):
    for rr in rows:
        h = fnv64(rr.encode('utf-8') + b'\n', h)
        ROWS[0] += 1
    return h


def fx_modules(rel, pnode):
    out = []
    mods = [(n.get('name'), n) for n in pnode.nodes if n.name == 'MODULE']
    names = [m[0] for m in mods]
    pname = (pnode.get('name') or '').replace('_', '.')
    engines = [m[1] for m in mods if m[0] in ('ModuleEngines', 'ModuleEnginesFX')]
    if 'FXModuleAnimateThrottle' in names and 'MultiModeEngine' not in names and len(engines) == 1:
        mod = [m[1] for m in mods if m[0] == 'FXModuleAnimateThrottle'][0]
        eng = engines[0]
        ename = mod.get('engineName')
        if ename in (None, '') or p_str(eng, 'engineID', 'Engine') == ename:
            for sc in ('flight', 'editor'):
                m = AnimThrottle(mod, eng)
                m.start()
                seq = []
                for k in range(0, 61):
                    th = f32((k % 21) / 20.0 if (k // 21) % 2 == 0 else 1.0 - (k % 21) / 20.0)
                    ign = 1 if k % 17 not in (0, 1, 2) else 0
                    flame = 1 if k % 23 == 5 else 0
                    cur = f32(th * 0.9)
                    fin = f32(th * 100.0)
                    scene = 'F' if sc == 'flight' else 'E'
                    if k == 50:
                        scene = 'N'
                    vessel = 0 if k == 55 else 1
                    seq.append('s %s %d %s %d %d %s %s' % (scene, vessel, hx(th), ign, flame, hx(cur), hx(fin)))
                h = 0xcbf29ce484222325
                digs = []
                for st in seq:
                    t = st.split(' ')
                    m.step(t[1], t[2] == '1', hexbits_to_float(t[3]), t[4] == '1', t[5] == '1', hexbits_to_float(t[6]), hexbits_to_float(t[7]))
                    h = hash_rows(['step ' + st, m.row()], h)
                    digs.append(h & 0xFFFFFFFF)
                out.append({'part': rel, 'name': pname, 'kind': 'animthrottle', 'variant': sc, 'steps': seq, 'digest': '%016x' % h, 'steps_digest': dig_hex(digs)})
    if 'FXModuleAnimateRCS' in names:
        mod = [m[1] for m in mods if m[0] == 'FXModuleAnimateRCS'][0]
        for aff in (True, False):
            m = AnimRCS(mod, rel)
            m.affect_time = aff
            seq = []
            for k in range(0, 40):
                f = [f32(((k * 3 + i * 7) % 11) / 10.0) for i in range(m.n)]
                scene = 'F' if k != 30 else 'E'
                seq.append('r %s %s' % (scene, ' '.join(hx(x) for x in f)))
            h = 0xcbf29ce484222325
            digs = []
            for st in seq:
                t = st.split(' ')
                m.step(t[1], [hexbits_to_float(x) for x in t[2:]])
                h = hash_rows(['step ' + st] + m.rows(), h)
                digs.append(h & 0xFFFFFFFF)
            h = hash_rows(['save ' + m.save()], h)
            out.append({'part': rel, 'name': pname, 'kind': 'animrcs', 'variant': 'affectTime' if aff else 'speed', 'steps': seq, 'digest': '%016x' % h,
                        'steps_digest': dig_hex(digs), 'n': m.n})
    for cls, node_name, movers_key in (('FXModuleLookAtConstraint', 'CONSTRAINLOOKFX', 'rotatorsName'), ('FXModuleConstrainPosition', 'CONSTRAINFX', 'moversName')):
        if cls not in names:
            continue
        mod = [m[1] for m in mods if m[0] == cls][0]
        mode = p_str(mod, 'trackingModeString', 'LateUpdate')
        mode_i = {'FixedUpdate': 0, 'Update': 1, 'LateUpdate': 2}[mode]
        objs = []
        for cn in mod.nodes:
            if cn.name == node_name:
                tn = cn.get('targetName') or ''
                mv = cn.get(movers_key) or ''
                tcount = FIX_count(rel, tn)
                mcount = FIX_count(rel, mv)
                if tcount > 0 and mcount >= 1:
                    objs.append((tn, mv, mcount))
        have_node = any(cn.name == node_name for cn in mod.nodes)
        match_pos = p_bool(mod, 'matchPosition', True)
        match_rot = p_bool(mod, 'matchRotation', True)
        seq = ['t %s' % 'ufl'[k % 3] for k in range(9)]
        h = 0xcbf29ce484222325
        digs = []
        for st in seq:
            call_mode = {'f': 0, 'u': 1, 'l': 2}[st.split(' ')[1]]
            rows = ['step ' + st, 'mode %d|%d|%d' % (mode_i, len(objs), 1 if have_node else 0)]
            if call_mode == mode_i:
                for tn, mv, mc in objs:
                    for j in range(mc):
                        if cls == 'FXModuleLookAtConstraint':
                            rows.append('track lookAt|%s|%s|%d|1|0|0' % (tn, mv, j))
                        else:
                            rows.append('track constrain|%s|%s|%d|0|%d|%d' % (tn, mv, j, 1 if match_pos else 0, 1 if match_rot else 0))
            h = hash_rows(rows, h)
            digs.append(h & 0xFFFFFFFF)
        out.append({'part': rel, 'name': pname, 'kind': 'lookat' if cls == 'FXModuleLookAtConstraint' else 'constrain', 'variant': mode, 'steps': seq,
                    'digest': '%016x' % h, 'steps_digest': dig_hex(digs), 'objects': len(objs)})
    return out


def dig_hex(digests):
    return ''.join('%08x' % d for d in digests)


def make_env(rel, eva=False, loading=False):
    return Env(rel, eva, loading)


def process(parts, dump=None):
    out_parts = []
    fxm = []
    totals = {'parts': 0, 'parts_with_effects': 0, 'effects': 0, 'behaviours': {}, 'unknown_classes': {}, 'steps': 0, 'groups': 0, 'parts_with_groups': 0,
              'collapsed_unnamed': 0}
    for rel, pnode, effs in parts:
        totals['parts'] += 1
        fixture = rel.startswith('fixture/')
        groups_present = False
        entry = {'part': rel, 'name': (pnode.get('name') or '').replace('_', '.'), 'scenarios': {}}
        if fixture:
            entry['fixture_cfg'] = FIXTURE_TEXT
        if effs:
            env0 = make_env(rel)
            lst0 = build_list(env0, effs)
            names = [e[0] for e in lst0.entries]
            for e in lst0.errors:
                totals['unknown_classes'][e] = totals['unknown_classes'].get(e, 0) + 1
            total_nodes = sum(len(cn.nodes) for en in effs for cn in en.nodes)
            beh = sum(len(e[1]) for e in lst0.entries)
            totals['parts_with_effects'] += 1
            totals['effects'] += len(names)
            totals['collapsed_unnamed'] += total_nodes - beh - len(lst0.errors)
            for _, l in lst0.entries:
                for b in l:
                    totals['behaviours'][b.NODE] = totals['behaviours'].get(b.NODE, 0) + 1
            entry['effects'] = names
            entry['behaviours'] = [[b.effect_name, b.NODE, b.instance_name if b.instance_name is not None else None] for _, l in lst0.entries for b in l]
            scen = {}
            specs = [('sweep', sweep_script(names), dict()), ('early', early_script(names), dict()), ('eva', sweep_script(names), dict(eva=True)),
                     ('loading', sweep_script(names), dict(loading=True))]
            for sname, steps, kw in specs:
                env = make_env(rel, **kw)
                lst = build_list(env, effs)
                h, digs, rows = run_steps(rel, env, lst, steps)
                totals['steps'] += len(steps)
                scen[sname] = {'kw': kw, 'steps': steps, 'digest': '%016x' % h, 'steps_digest': dig_hex(digs)}
                if dump and dump[0] == rel and dump[1] == sname:
                    for i, r in enumerate(rows):
                        print('--- step %d: %s' % (i, steps[i]))
                        print('\n'.join(r))
            # save text
            env = make_env(rel)
            lst = build_list(env, effs)
            sv = lst.save_rows()
            h = 0xcbf29ce484222325
            for r in sv:
                h = fnv64(r.encode('utf-8') + b'\n', h)
                ROWS[0] += 1
            scen['save'] = {'digest': '%016x' % h, 'rows': len(sv)}
            if dump and dump[0] == rel and dump[1] == 'save':
                print('\n'.join(sv))
            # engine / rcs scripts on parts whose modules use these effect names
            mods = [n.get('name') for n in pnode.nodes if n.name == 'MODULE']
            if 'ModuleEnginesFX' in mods or fixture:
                for resp in (True, False):
                    steps = engine_script(names)
                    env = make_env(rel)
                    lst = build_list(env, effs)
                    awake_all(pnode, lst)
                    lst.initialize()
                    lst.event_rows()
                    h, digs = run_engine(env, lst, resp, steps, module_params(pnode, 'ModuleEnginesFX'))
                    scen['engine_resp' if resp else 'engine_direct'] = {'steps': steps, 'digest': '%016x' % h, 'steps_digest': dig_hex(digs)}
                    totals['steps'] += len(steps)
            if 'ModuleRCSFX' in mods or fixture:
                steps = rcs_script()
                env = make_env(rel)
                lst = build_list(env, effs)
                awake_all(pnode, lst)
                lst.initialize()
                lst.event_rows()
                h, digs = run_rcs(env, lst, steps, module_params(pnode, 'ModuleRCSFX'))
                scen['rcs'] = {'steps': steps, 'digest': '%016x' % h, 'steps_digest': dig_hex(digs)}
                totals['steps'] += len(steps)
            entry['scenarios'] = scen
        # legacy groups
        if not fixture and any(k.startswith('fx') or k.startswith('sound') for k, _ in pnode.values):
            groups, order = build_groups(pnode)
            pa = Src(make_env(rel))
            pa.id = 0
            steps = group_script(order)
            h, digs = run_groups(pa, groups, order, steps)
            entry['scenarios']['groups'] = {'steps': steps, 'order': order, 'digest': '%016x' % h, 'steps_digest': dig_hex(digs)}
            totals['groups'] += len(order)
            totals['parts_with_groups'] += 1
            totals['steps'] += len(steps)
        if not fixture:
            fxm += fx_modules(rel, pnode)
        if entry['scenarios']:
            out_parts.append(entry)
    totals['fx_modules'] = {}
    for e in fxm:
        totals['fx_modules'][e['kind']] = totals['fx_modules'].get(e['kind'], 0) + 1
    return out_parts, totals, fxm


def main(argv):
    mirror = None
    dump = None
    i = 1
    while i < len(argv):
        if argv[i] == '--mirror':
            mirror = argv[i + 1]
            i += 2
        elif argv[i] == '--dump':
            dump = (argv[i + 1], argv[i + 2])
            i += 3
        else:
            raise SystemExit('unknown argument ' + argv[i])
    if mirror is None:
        raise SystemExit('--mirror <dir> required')
    parts = collect_parts(mirror) + fixture_parts()
    out_parts, totals, fxm = process(parts, dump)
    if dump:
        return
    totals['rows'] = ROWS[0]
    doc = {'schema': 'wpue16-effects/1', 'totals': totals, 'parts': out_parts, 'fx_modules': fxm}
    json.dump(doc, sys.stdout, indent=0, sort_keys=True, separators=(',', ':'))
    sys.stdout.write('\n')


if __name__ == '__main__':
    main(sys.argv)

"""Independent Python transcription of KSP's C# LibNoise subset (decompiled/KSP-Decompiled/Assembly-CSharp/LibNoise/),
used to derive the hand-checked expectations in tests/test_terrain.cpp. Python floats are IEEE doubles evaluated in
source order, so these values are what the C# computes (no shared code with the C++ port; the RandomVectors table is
read from the C# source).

    python -I tools/wp11/libnoise_reference.py <decompiled Assembly-CSharp dir>
"""
import math
import os
import re
import struct
import sys

M32 = 0xFFFFFFFF


def i32(v):
    v &= M32
    return v - (1 << 32) if v & 0x80000000 else v


def conv_i4(d):
    if not (-2147483649.0 < d < 2147483648.0):
        return -2147483648
    return int(d)  # truncation


def lattice(x):
    return conv_i4(x) if x > 0.0 else i32(conv_i4(x) - 1)


def load_vectors(cs_dir):
    s = open(os.path.join(cs_dir, "LibNoise", "GradientNoiseBasis.cs"), encoding="utf-8").read()
    body = re.search(r"RandomVectors = new double\[1024\]\s*\{(.*?)\};", s, re.S).group(1)
    return [float(x) for x in body.replace("\n", " ").split(",") if x.strip()]


RV = []


def scurve3(a):
    return a * a * (3.0 - 2.0 * a)


def scurve5(a):
    n = a * a * a
    n2 = n * a
    n3 = n2 * a
    return 6.0 * n3 - 15.0 * n2 + 10.0 * n


def lerp(n0, n1, a):
    return (1.0 - a) * n0 + a * n1


def gradient_noise(fx, fy, fz, ix, iy, iz, seed):
    ipart = i32(1619 * ix + 31337 * iy + 6971 * iz)
    num = (ipart + 1013 * seed) & M32
    num ^= num >> 8
    num &= 0xFF
    return (RV[num << 2] * (fx - ix) + RV[(num << 2) + 1] * (fy - iy) + RV[(num << 2) + 2] * (fz - iz)) * 2.12


def coherent(x, y, z, seed, q):
    n, n2, n3 = lattice(x), lattice(y), lattice(z)
    ix, iy, iz = i32(n + 1), i32(n2 + 1), i32(n3 + 1)
    f = {0: lambda a: a, 1: scurve3, 2: scurve5}[q]
    a, a2, a3 = f(x - n), f(y - n2), f(z - n3)
    g = gradient_noise
    c0 = lerp(g(x, y, z, n, n2, n3, seed), g(x, y, z, ix, n2, n3, seed), a)
    c1 = lerp(g(x, y, z, n, iy, n3, seed), g(x, y, z, ix, iy, n3, seed), a)
    c5 = lerp(c0, c1, a2)
    d0 = lerp(g(x, y, z, n, n2, iz, seed), g(x, y, z, ix, n2, iz, seed), a)
    d1 = lerp(g(x, y, z, n, iy, iz, seed), g(x, y, z, ix, iy, iz, seed), a)
    c6 = lerp(d0, d1, a2)
    return lerp(c5, c6, a3)


def perlin(x, y, z, freq, lac, pers, octaves, seed, q):
    num, amp = 0.0, 1.0
    x, y, z = x * freq, y * freq, z * freq
    for i in range(octaves):
        num += coherent(x, y, z, i32(seed + i), q) * amp
        x, y, z = x * lac, y * lac, z * lac
        amp *= pers
    return num


def billow(x, y, z, freq, lac, pers, octaves, seed, q):
    num, amp = 0.0, 1.0
    x, y, z = x * freq, y * freq, z * freq
    for i in range(octaves):
        v = 2.0 * abs(coherent(x, y, z, i32(seed + i), q)) - 1.0
        num += v * amp
        x, y, z = x * lac, y * lac, z * lac
        amp *= pers
    return num + 0.5


def ridged(x, y, z, freq, lac, octaves, seed, q):
    weights, f = [], 1.0
    for _ in range(30):
        weights.append(f ** -1.0)
        f *= lac
    x, y, z = x * freq, y * freq, z * freq
    num2, num3 = 0.0, 1.0
    for i in range(octaves):
        num = coherent(x, y, z, i32(seed + i) & 0x7FFFFFFF, q)
        num = 1.0 - abs(num)
        num *= num
        num *= num3
        num3 = num * 2.0
        if num3 > 1.0:
            num3 = 1.0
        if num3 < 0.0:
            num3 = 0.0
        num2 += num * weights[i]
        x, y, z = x * lac, y * lac, z * lac
    return num2 * 1.25 - 1.0


def int_value_noise(x, y, z, seed):
    num = i32(1619 * x + 31337 * y + 6971 * z + 1013 * seed) & 0x7FFFFFFF
    num = (num >> 13) ^ num
    return i32(num * i32(i32(num * num) * 60493 + 19990303) + 1376312589) & 0x7FFFFFFF


def value_noise(x, y, z, seed=0):
    return 1.0 - int_value_noise(x, y, z, seed) / 1073741824.0


def voronoi(x, y, z, freq, disp, seed, dist):
    x, y, z = x * freq, y * freq, z * freq
    n, n2, n3 = lattice(x), lattice(y), lattice(z)
    best, cx, cy, cz = 2147483647.0, 0.0, 0.0, 0.0
    for i in range(n3 - 2, n3 + 3):
        for j in range(n2 - 2, n2 + 3):
            for k in range(n - 2, n + 3):
                px = k + value_noise(k, j, i, seed)
                py = j + value_noise(k, j, i, i32(seed + 1))
                pz = i + value_noise(k, j, i, i32(seed + 2))
                dx, dy, dz = px - x, py - y, pz - z
                d = dx * dx + dy * dy + dz * dz
                if d < best:
                    best, cx, cy, cz = d, px, py, pz
    v = math.sqrt((cx - x) ** 2 + (cy - y) ** 2 + (cz - z) ** 2) * 1.7320508075688772 - 1.0 if dist else 0.0
    return v + disp * value_noise(lattice(cx), lattice(cy), lattice(cz))


def hx(d):
    return "0x%016X" % struct.unpack("<Q", struct.pack("<d", d))[0]


def main():
    RV.extend(load_vectors(sys.argv[1]))
    cases = [
        ("IntValueNoise(0,0,0,0)", float(int_value_noise(0, 0, 0, 0))),
        ("IntValueNoise(1,2,3,4)", float(int_value_noise(1, 2, 3, 4))),
        ("IntValueNoise(-7,100000,-3,1384047773)", float(int_value_noise(-7, 100000, -3, 1384047773))),
        ("ValueNoise(5,-6,7)", value_noise(5, -6, 7)),
        ("Coherent(0.5,0.25,0.75,0,Low)", coherent(0.5, 0.25, 0.75, 0, 0)),
        ("Coherent(-1.3,2.7,0.1,42,Standard)", coherent(-1.3, 2.7, 0.1, 42, 1)),
        ("Coherent(10.6,-3.2,7.9,-5,High)", coherent(10.6, -3.2, 7.9, -5, 2)),
        ("Coherent(1,2,3,7,Std) lattice", coherent(1.0, 2.0, 3.0, 7, 1)),
        ("Perlin(0.3,-0.4,0.5;f2 l2 p0.5 o6 s123 Std)", perlin(0.3, -0.4, 0.5, 2.0, 2.0, 0.5, 6, 123, 1)),
        ("Billow(0.3,-0.4,0.5;f2 l2 p0.5 o4 s7 Low)", billow(0.3, -0.4, 0.5, 2.0, 2.0, 0.5, 4, 7, 0)),
        ("Ridged(0.6,0.0,-0.8;f48 l2 o6 s2147483646 Low)", ridged(0.6, 0.0, -0.8, 48.0, 2.0, 6, 2147483646, 0)),
        ("Voronoi(0.6,0.0,-0.8;f22 d0 s824 dist)", voronoi(0.6, 0.0, -0.8, 22.0, 0.0, 824, True)),
        ("Voronoi(0.1,0.2,0.3;f3 d1 s5 nodist)", voronoi(0.1, 0.2, 0.3, 3.0, 1.0, 5, False)),
    ]
    for name, v in cases:
        print("%-48s %s  %r" % (name, hx(v), v))


if __name__ == "__main__":
    main()

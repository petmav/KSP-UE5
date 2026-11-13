"""Texture2D -> raw files for the WP-UE-06 extractor (UnityPy for the pixel bytes, this module for the container formats).

Nothing is re-encoded and nothing is resampled:
* block-compressed formats (DXT1, DXT5, BC4, BC7 ...) are written as a DDS: header + the mip chain exactly as Unity stores it;
* 8-bit uncompressed formats (Alpha8, RGB24, RGBA32, ARGB32, BGRA32, 4444, 565) become a PNG of mip 0 (a lossless channel reorder /
  bit expansion; the stored smaller mips are not written);
* half / float formats become a DDS with a DX10 header, all mips;
* Crunched textures are unpacked to their DXT blocks by texture2ddecoder (the codec Unity itself uses; lossless).
Row order: every file keeps Unity's memory order (the first row is the BOTTOM row, v = 0). Mesh UVs are Unity's too, so a mesh and
its texture agree without any flip; an image viewer shows these files upside down.
"""
import struct
import zlib

# Unity TextureFormat id -> (name, kind, bytes per pixel or per 4x4 block, DXGI format for the DX10 header)
FORMATS = {
    1: ("Alpha8", "raw", 1, 65),
    2: ("ARGB4444", "raw", 2, None),
    3: ("RGB24", "raw", 3, None),
    4: ("RGBA32", "raw", 4, 28),
    5: ("ARGB32", "raw", 4, None),
    7: ("RGB565", "raw", 2, 85),
    9: ("R16", "raw", 2, 56),
    10: ("DXT1", "block", 8, None),
    12: ("DXT5", "block", 16, None),
    13: ("RGBA4444", "raw", 2, None),
    14: ("BGRA32", "raw", 4, 87),
    15: ("RHalf", "raw", 2, 54),
    16: ("RGHalf", "raw", 4, 34),
    17: ("RGBAHalf", "raw", 8, 10),
    18: ("RFloat", "raw", 4, 41),
    19: ("RGFloat", "raw", 8, 16),
    20: ("RGBAFloat", "raw", 16, 2),
    24: ("BC6H", "block", 16, 95),
    25: ("BC7", "block", 16, 98),
    26: ("BC4", "block", 8, 80),
    27: ("BC5", "block", 16, 83),
    28: ("DXT1Crunched", "crunch", 8, None),
    29: ("DXT5Crunched", "crunch", 16, None),
}
CRUNCH_TARGET = {28: 10, 29: 12}
PNG_FORMATS = {1, 2, 3, 4, 5, 13, 14}


class Undecodable(Exception):
    pass


def mip_sizes(width, height, mips, fmt):
    """Byte size of every mip level (Unity order: largest first)."""
    _name, kind, bpp, _dxgi = FORMATS[fmt]
    out = []
    for i in range(mips):
        w, h = max(1, width >> i), max(1, height >> i)
        if kind in ("block", "crunch"):
            out.append(max(1, (w + 3) // 4) * max(1, (h + 3) // 4) * bpp)
        else:
            out.append(w * h * bpp)
    return out


def dds(width, height, mips, fmt, data, array_size=1):
    """DDS file bytes: header (+ DX10 header when needed) + `data` verbatim."""
    name, kind, bpp, dxgi = FORMATS[fmt]
    if kind == "crunch":
        fmt = CRUNCH_TARGET[fmt]
        name, kind, bpp, dxgi = FORMATS[fmt]
    flags = 0x1 | 0x2 | 0x4 | 0x1000 | (0x20000 if mips > 1 else 0)
    pitch_or_linear = 0
    if kind == "block":
        flags |= 0x80000
        pitch_or_linear = mip_sizes(width, height, 1, fmt)[0]
    else:
        flags |= 0x8
        pitch_or_linear = width * bpp
    use_dx10 = array_size > 1 or fmt not in (10, 12)
    if use_dx10 and dxgi is None:
        raise Undecodable("no DXGI format for TextureFormat %s" % name)
    # DDS_PIXELFORMAT: size, flags FOURCC, fourCC
    fourcc = b"DX10" if use_dx10 else (b"DXT1" if fmt == 10 else b"DXT5")
    pf = struct.pack("<II4sIIIII", 32, 0x4, fourcc, 0, 0, 0, 0, 0)
    caps = 0x1000 | (0x8 | 0x400000 if mips > 1 else 0)
    header = struct.pack("<4sIIIIIII11I", b"DDS ", 124, flags, height, width, pitch_or_linear, 0, mips, *([0] * 11))
    header += pf + struct.pack("<IIIII", caps, 0, 0, 0, 0)
    assert len(header) == 128, len(header)
    if use_dx10:
        header += struct.pack("<IIIII", dxgi, 3, 0, array_size, 0)
    return header + bytes(data)


def _chunk(tag, payload):
    c = struct.pack(">I", len(payload)) + tag + payload
    return c + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)


def png(width, height, color_type, rows):
    """PNG file bytes, 8 bit, filter 0, zlib level 9. `rows` is bytes of width * channels * height (top row first = the file's order)."""
    ch = {0: 1, 2: 3, 6: 4}[color_type]
    stride = width * ch
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        raw += rows[y * stride:(y + 1) * stride]
    return (b"\x89PNG\r\n\x1a\n" + _chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, color_type, 0, 0, 0))
            + _chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + _chunk(b"IEND", b""))


def to_png_pixels(fmt, w, h, data):
    """(color_type, pixel bytes) of mip 0 in the file's channel order. Rows stay in Unity memory order."""
    n = w * h
    if fmt == 1:
        return 0, bytes(data[:n])
    if fmt == 3:
        return 2, bytes(data[:n * 3])
    if fmt == 4:
        return 6, bytes(data[:n * 4])
    d = memoryview(data)
    out = bytearray(n * 4)
    if fmt == 5:          # ARGB32 bytes A R G B
        out[0::4], out[1::4], out[2::4], out[3::4] = d[1:n * 4:4], d[2:n * 4:4], d[3:n * 4:4], d[0:n * 4:4]
    elif fmt == 14:       # BGRA32 bytes B G R A
        out[0::4], out[1::4], out[2::4], out[3::4] = d[2:n * 4:4], d[1:n * 4:4], d[0:n * 4:4], d[3:n * 4:4]
    elif fmt in (2, 13):
        # Unity packs 16-bit texels little endian. ARGB4444: A nibble 15..12, R 11..8, G 7..4, B 3..0. RGBA4444: R 15..12, G 11..8,
        # B 7..4, A 3..0 (Unity TextureFormat docs). Each nibble v becomes v * 17.
        for i in range(n):
            v = d[2 * i] | (d[2 * i + 1] << 8)
            nib = ((v >> 12) & 15, (v >> 8) & 15, (v >> 4) & 15, v & 15)
            a, r, g, b = nib if fmt == 2 else (nib[3], nib[0], nib[1], nib[2])
            out[4 * i:4 * i + 4] = bytes((r * 17, g * 17, b * 17, a * 17))
    else:
        raise Undecodable("png from format %d" % fmt)
    return 6, bytes(out)

// WP-UE-05: texture header decode. See ksp/parts/texture_header.hpp. Spec: Assembly-CSharp/DatabaseLoaderTexture_*.cs, MBMReader.cs,
// TGAHeader.cs, TGAImage.cs, DDSHeaders/*.
#include "ksp/parts/texture_header.hpp"

#include <cstring>

namespace ksp::tex {

namespace {

bool EndsWith(const std::string& s, const char* suffix) {
    const size_t n = std::strlen(suffix);
    return s.size() >= n && s.compare(s.size() - n, n, suffix) == 0;
}

std::string Lower(std::string s) {
    for (char& c : s)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return s;
}

// Path.GetFileNameWithoutExtension / GetExtension on '/' and '\\' separated names
std::string BaseName(const std::string& path) {
    size_t start = 0;
    for (size_t i = 0; i < path.size(); ++i)
        if (path[i] == '/' || path[i] == '\\') start = i + 1;
    return path.substr(start);
}
std::string StemOf(const std::string& path) {
    std::string f = BaseName(path);
    const size_t dot = f.rfind('.');
    if (dot != std::string::npos) f.resize(dot);
    return f;
}
std::string ExtensionOf(const std::string& path) {
    const std::string f = BaseName(path);
    const size_t dot = f.rfind('.');
    return dot == std::string::npos ? std::string() : Lower(f.substr(dot + 1));
}

struct Buf {
    const uint8_t* d;
    size_t n;
    bool has(size_t off, size_t len) const { return off <= n && len <= n - off; }
    uint32_t le32(size_t off) const {
        return static_cast<uint32_t>(d[off]) | (static_cast<uint32_t>(d[off + 1]) << 8) | (static_cast<uint32_t>(d[off + 2]) << 16) |
               (static_cast<uint32_t>(d[off + 3]) << 24);
    }
    uint32_t be32(size_t off) const {
        return (static_cast<uint32_t>(d[off]) << 24) | (static_cast<uint32_t>(d[off + 1]) << 16) | (static_cast<uint32_t>(d[off + 2]) << 8) |
               static_cast<uint32_t>(d[off + 3]);
    }
    uint32_t le16(size_t off) const { return static_cast<uint32_t>(d[off]) | (static_cast<uint32_t>(d[off + 1]) << 8); }
};

// floor(log2(max(w, h))) + 1: the levels of a full mip chain
int32_t FullChain(int32_t w, int32_t h) {
    int32_t m = w > h ? w : h, n = 1;
    while (m > 1) {
        m >>= 1;
        ++n;
    }
    return n;
}

uint32_t FourCC(const char* s) {
    return static_cast<uint32_t>(static_cast<uint8_t>(s[0])) | (static_cast<uint32_t>(static_cast<uint8_t>(s[1])) << 8) |
           (static_cast<uint32_t>(static_cast<uint8_t>(s[2])) << 16) | (static_cast<uint32_t>(static_cast<uint8_t>(s[3])) << 24);
}

// Texture2D.Compress (DatabaseLoaderTexture_PNG.cs:57, MBMReader.cs:105): applies only to dimensions that are multiples of 4;
// RGB24 -> DXT1, alpha formats -> DXT5 [Unity behaviour, INFERRED]
int32_t CompressedOf(int32_t format) { return format == kRGB24 ? kDXT1 : kDXT5; }

void Finish(Header& h, uint64_t fileSize, uint64_t size) {
    const uint64_t total = fileSize ? fileSize : size;
    h.dataBytes = total > h.dataOffset ? total - h.dataOffset : 0;
}

// ---- DDS: DatabaseLoaderTexture_DDS.cs:29 Read ---------------------------------------------------------------------------------

void DecodeDds(const Buf& b, Header& h) {
    h.container = Container::DDS;
    if (!b.has(0, 4) || b.le32(0) != FourCC("DDS ")) {  // :32
        h.error = "DDS: File is not a DDS format file!";
        return;
    }
    if (!b.has(0, 128)) {
        h.error = "DDS: truncated header";
        return;
    }
    h.ddsHeaderFlags = b.le32(8);
    h.height = static_cast<int32_t>(b.le32(12));
    h.width = static_cast<int32_t>(b.le32(16));
    h.ddsMipMapCount = b.le32(28);
    h.ddsPixelFormatFlags = b.le32(80);
    h.ddsFourCC = b.le32(84);
    h.ddsRgbBitCount = b.le32(88);
    h.ddsCaps = b.le32(108);
    h.dataOffset = 128;
    if (h.ddsFourCC == FourCC("DX10")) {  // :38 DDSHeaderDX10 is read and ignored
        if (!b.has(128, 20)) {
            h.error = "DDS: truncated DX10 header";
            return;
        }
        h.ddsDxgiFormat = b.le32(128);
        h.dataOffset = 148;
    }
    h.valid = true;
    h.mipChain = (h.ddsCaps & 0x400000u) != 0 ? 1 : 0;                                   // :42 DDSPixelFormatCaps.MIPMAP
    h.isNormalMap = (h.ddsPixelFormatFlags & 0x80000u) != 0 || (h.ddsPixelFormatFlags & 0x80000000u) != 0;  // :43
    h.isReadable = false;                                                                  // :16 TextureInfo(..., isReadable: false, isCompressed: true)
    h.isCompressedFlag = true;
    h.compressed = true;
    if (h.ddsFourCC == FourCC("DXT1")) {  // :45
        h.sourceFormat = h.unityFormat = kDXT1;
        h.loadable = true;
    } else if (h.ddsFourCC == FourCC("DXT5")) {  // :55
        h.sourceFormat = h.unityFormat = kDXT5;
        h.loadable = true;
    } else if (h.ddsFourCC == FourCC("DXT3")) {  // :51
        h.error = "DDS: DXT3 format is NOT supported. Use DXT5";
    } else if (h.ddsFourCC == FourCC("DXT2")) {  // :61
        h.error = "DDS: DXT2 is not supported!";
    } else if (h.ddsFourCC == FourCC("DXT4")) {  // :65
        h.error = "DDS: DXT4 is not supported!";
    } else if (h.ddsFourCC == FourCC("DX10")) {  // :69
        h.error = "DDS: DX10 formats not supported";
    } else {
        h.error = "DDS: unsupported pixel format (no FourCC branch matches: the loader returns a null texture)";
    }
    h.mipCount = h.mipChain == 1 ? FullChain(h.width, h.height) : 1;
    if (h.loadable) {
        // Texture2D(w, h, DXT1 / DXT5, mipChain) + LoadRawTextureData(rest of the file): Unity wants exactly the chain's blocks
        const uint64_t block = h.unityFormat == kDXT1 ? 8 : 16;
        uint64_t total = 0;
        int64_t w = h.width, hh = h.height;
        for (int32_t i = 0; i < h.mipCount; ++i) {
            const int64_t bw = (w + 3) / 4 > 1 ? (w + 3) / 4 : 1, bh = (hh + 3) / 4 > 1 ? (hh + 3) / 4 : 1;
            total += static_cast<uint64_t>(bw * bh) * block;
            w = w > 1 ? w / 2 : 1;
            hh = hh > 1 ? hh / 2 : 1;
        }
        h.expectedDataBytes = total;
    }
}

// ---- MBM: MBMReader.cs:25 ReadTexture2D ---------------------------------------------------------------------------------------

void DecodeMbm(const Buf& b, Header& h) {
    h.container = Container::MBM;
    size_t off = 0;
    uint32_t len = 0;
    int shift = 0;
    for (;;) {  // BinaryReader.ReadString length prefix (:27)
        if (!b.has(off, 1) || shift > 35) {
            h.error = "MBM: truncated header";
            return;
        }
        const uint8_t byte = b.d[off++];
        len |= static_cast<uint32_t>(byte & 0x7F) << shift;
        if (!(byte & 0x80)) break;
        shift += 7;
    }
    if (!b.has(off, len)) {
        h.error = "MBM: truncated header";
        return;
    }
    off += len;
    if (!b.has(off, 16)) {
        h.error = "MBM: truncated header";
        return;
    }
    h.width = static_cast<int32_t>(b.le32(off));       // :28
    h.height = static_cast<int32_t>(b.le32(off + 4));  // :29
    h.mbmTextureType = static_cast<int32_t>(b.le32(off + 8));    // :30
    h.mbmBitsPerPixel = static_cast<int32_t>(b.le32(off + 12));  // :31
    h.dataOffset = off + 16;
    h.valid = true;
    h.mipChain = 1;           // :45 / :49 / :85 mipChain: true
    h.isCompressedFlag = true;  // DatabaseLoaderTexture_MBM.cs:14 TextureInfo(..., isReadable: true, isCompressed: true)
    h.isReadable = true;
    const int64_t pixels = static_cast<int64_t>(h.width) * h.height;
    if (h.mbmTextureType == 0) {  // TextureType.Texture :38
        if (h.mbmBitsPerPixel == 32) {
            h.sourceFormat = kRGBA32;
            h.hasAlpha = true;
            h.expectedDataBytes = static_cast<uint64_t>(pixels) * 4;
        } else if (h.mbmBitsPerPixel == 24) {
            h.sourceFormat = kRGB24;
            h.expectedDataBytes = static_cast<uint64_t>(pixels) * 3;
        } else {
            h.error = "MBM: bits per pixel other than 24 / 32: texture2D stays null, the loader fails";
            return;
        }
        h.isNormalMap = false;  // :101
        h.loadable = true;
        // :103 compress: true (DatabaseLoaderTexture_MBM.cs:11) -> Compress(false) then Apply
        if (h.width % 4 == 0 && h.height % 4 == 0) {
            h.compressed = true;
            h.unityFormat = CompressedOf(h.sourceFormat);
        } else {
            h.unityFormat = h.sourceFormat;
        }
    } else if (h.mbmTextureType == 1) {  // TextureType.NormalMap :83
        h.sourceFormat = h.unityFormat = kARGB32;
        h.isNormalMap = true;  // :111, no Compress
        h.hasAlpha = true;
        h.loadable = true;
        h.expectedDataBytes = static_cast<uint64_t>(pixels) * 4;
    } else {
        h.error = "MBM: unknown texture type: texture2D stays null, the loader fails";
        return;
    }
    h.mipCount = FullChain(h.width, h.height);
}

// ---- PNG / JPEG headers (the Unity decoders themselves are not in the decompile) -----------------------------------------------

struct Image {
    bool ok = false;
    bool isPng = false;
    int32_t width = 0, height = 0, colorType = -1, bitDepth = -1;
    bool alpha = false;
    std::string error;
};

Image SniffPng(const Buf& b) {
    Image im;
    static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    if (!b.has(0, 33) || std::memcmp(b.d, sig, 8) != 0 || std::memcmp(b.d + 12, "IHDR", 4) != 0) {
        im.error = "PNG: no IHDR";
        return im;
    }
    im.isPng = true;
    im.width = static_cast<int32_t>(b.be32(16));
    im.height = static_cast<int32_t>(b.be32(20));
    im.bitDepth = b.d[24];
    im.colorType = b.d[25];
    im.alpha = im.colorType == 4 || im.colorType == 6;
    size_t off = 8;  // chunks until IDAT: a tRNS chunk adds an alpha channel
    while (b.has(off, 8)) {
        const uint32_t len = b.be32(off);
        const bool idat = std::memcmp(b.d + off + 4, "IDAT", 4) == 0;
        if (idat) break;
        if (std::memcmp(b.d + off + 4, "tRNS", 4) == 0) im.alpha = true;
        if (static_cast<uint64_t>(len) + 12 > b.n - off) break;
        off += static_cast<size_t>(len) + 12;
    }
    im.ok = true;
    return im;
}

Image SniffJpeg(const Buf& b) {
    Image im;
    if (!b.has(0, 4) || b.d[0] != 0xFF || b.d[1] != 0xD8) {
        im.error = "JPEG: no SOI";
        return im;
    }
    size_t off = 2;
    while (b.has(off, 4)) {
        if (b.d[off] != 0xFF) {
            ++off;
            continue;
        }
        const uint8_t m = b.d[off + 1];
        if (m == 0xFF) {
            ++off;
            continue;
        }
        if (m == 0xD8 || m == 0x01 || (m >= 0xD0 && m <= 0xD7)) {
            off += 2;
            continue;
        }
        const bool sof = m >= 0xC0 && m <= 0xCF && m != 0xC4 && m != 0xC8 && m != 0xCC;
        if (sof) {
            if (!b.has(off, 9)) break;
            im.height = static_cast<int32_t>((static_cast<uint32_t>(b.d[off + 5]) << 8) | b.d[off + 6]);
            im.width = static_cast<int32_t>((static_cast<uint32_t>(b.d[off + 7]) << 8) | b.d[off + 8]);
            im.ok = true;
            return im;
        }
        const size_t len = (static_cast<size_t>(b.d[off + 2]) << 8) | b.d[off + 3];
        off += 2 + len;
    }
    im.error = "JPEG: no SOF marker in the buffer";
    return im;
}

// GameDatabase.BitmapToUnityNormalMap (:616): RGBA32, mip chain, not readable
void NormalMapResult(Header& h) {
    h.nrmSuffix = true;
    h.sourceFormat = kRGBA32;
    h.unityFormat = kRGBA32;
    h.compressed = false;
    h.mipChain = 1;
    h.isNormalMap = true;
    h.isReadable = false;
    h.isCompressedFlag = true;  // PNG / JPEG / TGA pass isCompressed: true; TRUECOLOR overrides with false
}

bool HasDir(const std::string& name, const char* dir) {
    std::string n = name;
    for (char& c : n)
        if (c == '\\') c = '/';
    return n.find(std::string("/") + dir + "/") != std::string::npos;
}

// DatabaseLoaderTexture_PNG.cs:9
void DecodePng(const Buf& b, const std::string& name, Header& h) {
    h.container = Container::PNG;
    const Image im = SniffPng(b);
    if (!im.ok) {
        h.error = im.error;
        return;
    }
    h.valid = h.loadable = true;
    h.width = im.width;
    h.height = im.height;
    h.pngColorType = im.colorType;
    h.pngBitDepth = im.bitDepth;
    h.hasAlpha = im.alpha;
    h.dataOffset = 8;
    const int32_t decoded = im.alpha ? kRGBA32 : kRGB24;  // UnityWebRequestTexture: INFERRED
    if (EndsWith(StemOf(name), "NRM")) {                   // :25
        NormalMapResult(h);
        h.mipCount = FullChain(h.width, h.height);
        return;
    }
    h.isNormalMap = false;
    h.isReadable = true;  // :66
    h.isCompressedFlag = true;
    h.sourceFormat = decoded;
    const bool icons = HasDir(name, "Icons") || HasDir(name, "Tutorials") || HasDir(name, "SimpleIcons");  // :34-42
    // :44-50 the Flags directory re-creates the texture with mipChain: true: the same mip state as the default below
    int32_t fmt = decoded;
    h.mipChain = 1;  // INFERRED: the decoded texture has a mip chain (Apply(updateMipmaps: true), :65)
    if (icons) {
        fmt = kARGB32;  // :53 new Texture2D(w, h, ARGB32, mipChain: false)
        h.mipChain = 0;
    }
    h.unityFormat = fmt;
    if (h.width % 4 == 0 && h.height % 4 == 0) {  // :57
        h.compressed = true;
        h.unityFormat = CompressedOf(fmt);
    }
    h.mipCount = h.mipChain == 1 ? FullChain(h.width, h.height) : 1;
}

// DatabaseLoaderTexture_JPEG.cs:9
void DecodeJpeg(const Buf& b, const std::string& name, Header& h) {
    h.container = Container::JPEG;
    const Image im = SniffJpeg(b);
    if (!im.ok) {
        h.error = im.error;
        return;
    }
    h.valid = h.loadable = true;
    h.width = im.width;
    h.height = im.height;
    if (EndsWith(StemOf(name), "NRM")) {  // :24
        NormalMapResult(h);
        h.mipCount = FullChain(h.width, h.height);
        return;
    }
    h.sourceFormat = h.unityFormat = kRGB24;  // INFERRED
    h.isReadable = true;                      // :42
    h.isCompressedFlag = true;
    h.mipChain = 1;  // INFERRED
    if (h.width % 4 == 0 && h.height % 4 == 0) {  // :33
        h.compressed = true;
        h.unityFormat = kDXT1;
    }
    h.mipCount = FullChain(h.width, h.height);
}

// DatabaseLoaderTexture_TGA.cs:8 + TGAHeader.cs:17 + TGAImage.cs:42 / :212
void DecodeTga(const Buf& b, const std::string& name, Header& h) {
    h.container = Container::TGA;
    if (!b.has(0, 18)) {
        h.error = "TGA invalid length";
        return;
    }
    h.tgaImageType = b.d[2];
    h.width = static_cast<int32_t>(b.le16(12));
    h.height = static_cast<int32_t>(b.le16(14));
    h.tgaPixelDepth = b.d[16];
    h.dataOffset = 18;  // ReadTrueColorImage starts at 18 (the id field is not skipped)
    h.valid = true;
    if (h.tgaImageType != 2 && h.tgaImageType != 10) {  // TGAImage.cs:44-52
        h.error = "TGA: image type is not supported";
        return;
    }
    const int32_t bpp = h.tgaPixelDepth / 8;
    if (bpp == 4) {
        h.sourceFormat = kRGBA32;
        h.hasAlpha = true;
    } else if (bpp == 3) {
        h.sourceFormat = kRGB24;
    } else {
        h.error = "Cannot create texture: Header denotes incorrect format";  // :237
        return;
    }
    h.loadable = true;
    h.mipChain = 1;  // :12 CreateTexture(mipmap: true, ..., compress: true, compressHighQuality: false): no Compress call
    h.mipCount = FullChain(h.width, h.height);
    h.unityFormat = h.sourceFormat;
    if (h.tgaImageType == 2) h.expectedDataBytes = static_cast<uint64_t>(h.width) * h.height * (bpp == 4 ? 4 : 3);
    if (EndsWith(StemOf(name), "NRM")) {  // :16
        NormalMapResult(h);
        return;
    }
    h.isNormalMap = false;
    h.isReadable = true;  // :23
    h.isCompressedFlag = true;
}

// DatabaseLoaderTexture_TRUECOLOR.cs:8: LoadImage on a 1x1 ARGB32 texture without mips
void DecodeTrueColor(const Buf& b, const std::string& name, Header& h) {
    h.container = Container::TRUECOLOR;
    Image im = SniffPng(b);
    if (!im.ok) im = SniffJpeg(b);
    if (!im.ok) {
        h.error = "Texture load error (neither a PNG nor a JPEG header)";
        return;
    }
    h.valid = h.loadable = true;
    h.width = im.width;
    h.height = im.height;
    h.pngColorType = im.colorType;
    h.pngBitDepth = im.bitDepth;
    h.hasAlpha = im.alpha;
    h.dataOffset = im.isPng ? 8 : 2;
    if (EndsWith(StemOf(name), "NRM")) {  // :18
        NormalMapResult(h);
        h.isCompressedFlag = false;  // :20 isCompressed: false
        h.mipCount = FullChain(h.width, h.height);
        return;
    }
    h.sourceFormat = h.unityFormat = im.alpha ? kRGBA32 : kRGB24;  // INFERRED (LoadImage replaces the ARGB32 placeholder)
    h.mipChain = 0;                                                  // :11 mipChain: false (INFERRED to survive LoadImage)
    h.mipCount = 1;
    h.isNormalMap = false;
    h.isReadable = false;  // :27
    h.isCompressedFlag = false;
}

}  // namespace

Header DecodeHeader(const std::string& name, const uint8_t* data, size_t size, uint64_t fileSize) {
    Header h;
    const Buf b{data, size};
    const std::string ext = ExtensionOf(name);
    if (ext == "dds") DecodeDds(b, h);
    else if (ext == "mbm") DecodeMbm(b, h);
    else if (ext == "png") DecodePng(b, name, h);
    else if (ext == "jpg" || ext == "jpeg") DecodeJpeg(b, name, h);
    else if (ext == "tga") DecodeTga(b, name, h);
    else if (ext == "truecolor") DecodeTrueColor(b, name, h);
    else h.error = "no texture loader for the extension '" + ext + "'";
    if (h.valid) Finish(h, fileSize, size);
    return h;
}

}  // namespace ksp::tex

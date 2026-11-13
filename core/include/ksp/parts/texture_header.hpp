// WP-UE-05: texture header decode for the formats KSP's GameDatabase loads from GameData, and the Unity format each loader creates.
//
// Spec (Assembly-CSharp/): DatabaseLoaderTexture_DDS.cs:29 Read (DDSHeaders/*: DDSHeader, DDSPixelFormat, DDSHeaderDX10, DDSValues),
// DatabaseLoaderTexture_MBM.cs + MBMReader.cs:25 ReadTexture2D, DatabaseLoaderTexture_PNG.cs:9, DatabaseLoaderTexture_JPEG.cs:9,
// DatabaseLoaderTexture_TGA.cs:8 + TGAHeader.cs + TGAImage.cs:212 CreateTexture, DatabaseLoaderTexture_TRUECOLOR.cs:8,
// GameDatabase.cs:616 BitmapToUnityNormalMap (the "...NRM" file name rule).
//
// Only HEADERS are decoded (the GPU upload is shell code). What the Unity decoders do behind UnityWebRequestTexture / LoadImage
// (PNG, JPEG, TRUECOLOR: RGB24 versus RGBA32, whether the texture has a mip chain) is not in the decompile: those fields are INFERRED
// from the loader code and marked below; they are not gated against Unity.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace ksp::tex {

enum class Container : int32_t { Unknown = 0, DDS, MBM, PNG, JPEG, TGA, TRUECOLOR };

// UnityEngine.TextureFormat values
enum UnityFormat : int32_t { kFormatNone = 0, kAlpha8 = 1, kRGB24 = 3, kRGBA32 = 4, kARGB32 = 5, kDXT1 = 10, kDXT5 = 12 };

struct Header {
    Container container = Container::Unknown;
    bool valid = false;     // the header parsed (dimensions are meaningful)
    bool loadable = false;  // the KSP loader would hand back a Texture2D (DXT2 / DXT3 / DXT4 / DX10 DDS, MBM bits other than 24 / 32 ... return null)
    std::string error;      // why valid / loadable is false (the loader's own message where it has one)
    int32_t width = 0, height = 0;
    int32_t sourceFormat = kFormatNone;  // the Texture2D format the loader creates first (DDS: the stored DXT format)
    int32_t unityFormat = kFormatNone;   // the format after the loader's own Compress step (see `compressed`)
    bool compressed = false;             // the loader compresses (DDS: stored compressed; others: Compress when width and height are multiples of 4)
    int32_t mipChain = -1;               // Texture2D mipChain: 1 / 0; -1 = not decided by the loader code (Unity decoder default, INFERRED 1 is likely)
    int32_t mipCount = 0;                // levels of the full chain when mipChain == 1 (floor(log2(max(w, h))) + 1), 1 when 0; 0 when unknown
    bool isNormalMap = false;            // GameDatabase.TextureInfo.isNormalMap as the loader sets it
    bool isReadable = false;             // TextureInfo.isReadable as the loader sets it
    bool isCompressedFlag = false;       // TextureInfo.isCompressed as the loader sets it
    bool nrmSuffix = false;              // file name (without extension) ends with "NRM": BitmapToUnityNormalMap (RGBA32, mips, not readable)
    // DDS
    uint32_t ddsHeaderFlags = 0, ddsPixelFormatFlags = 0, ddsFourCC = 0, ddsCaps = 0, ddsMipMapCount = 0, ddsRgbBitCount = 0;
    uint32_t ddsDxgiFormat = 0;  // DX10 header (0 when absent)
    // MBM
    int32_t mbmTextureType = -1, mbmBitsPerPixel = -1;  // PartToolsLib.TextureType (0 Texture, 1 NormalMap), bits per pixel
    // PNG / JPEG / TRUECOLOR
    int32_t pngColorType = -1, pngBitDepth = -1;
    bool hasAlpha = false;  // PNG: colour type 4 / 6 or a tRNS chunk before IDAT; TGA: 32 bit; JPEG never
    // TGA
    int32_t tgaImageType = -1, tgaPixelDepth = -1;
    // payload
    uint64_t dataOffset = 0;         // first byte after the header(s) the loader reads
    uint64_t dataBytes = 0;          // fileSize - dataOffset (0 when the file is shorter)
    uint64_t expectedDataBytes = 0;  // DDS / MBM / TGA raw: the byte count Unity needs for LoadRawTextureData / the pixel loop (0 = not applicable)
};

// Decodes the header of `data[0..size)`, which holds (at least the start of) the file `name` of `fileSize` bytes (0 = size). The
// loader is picked by the extension of `name` (DatabaseLoaderAttrib: dds, mbm, png, jpg, jpeg, tga, truecolor; case-insensitive);
// `name` is also where the NRM suffix and the Icons / Tutorials / SimpleIcons / Flags directory rules come from (PNG loader).
// Never throws: a header that cannot be parsed has valid == false and `error` set.
Header DecodeHeader(const std::string& name, const uint8_t* data, size_t size, uint64_t fileSize = 0);

}  // namespace ksp::tex

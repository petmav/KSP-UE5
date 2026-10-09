// WP-11: MapSO (Assembly-CSharp/MapSO.cs), the ScriptableObject KSP samples height maps and decals from.
//
// The pixel array is MapSO._data exactly as serialized in the game's Unity asset (no texture decode involved):
// _bpp bytes per pixel, row-major from y = 0, PixelIndex(x, y) = x * _bpp + y * _rowWidth. kspcore loads it from the
// extracted copy in core/data/terrain/maps/<key>.bin (tools/wp11/extract_terrain.py; origin asset file + PathID +
// sha256 in core/data/terrain/manifest.json).
//
// Decoding as KSP does it (managed float code, Mono rule: see ksp/dotnet/mono_float.hpp):
//   * byte -> float is `Byte2Float * (float)(int)b` with Byte2Float = 0.003921569f (not 1/255);
//   * GetPixelFloat(u, v) on a 1-byte map bilinearly blends GreyFloat with Mathf.Lerp (t clamped to [0, 1]); on wider
//     maps it averages all channels of a pixel first (GetPixelFloat(int, int));
//   * GetPixelHeightAlpha: 2 bytes per pixel = (height, alpha) = channels 0 and 1, 4 bytes = channels 0 and 3, otherwise
//     (channel 0, 1); blended with HeightAlpha.Lerp (no clamp). These "16-bit" decal maps are two 8-bit channels;
//     there is no 16-bit height decode in KSP 1.12.5.
//   * ConstructBilinearCoords wraps u, v with `|x - floor(x)|`; the upper texel wraps to 0 only when it equals the
//     width/height (Ceiling of an exact integer coordinate stays on the same texel); u = 1.0 maps to texel 0.
//     The float overload (MapDecal) and the double overload (VertexHeightMap, PQSLandControl) round differently.
//   * GetPixelByte(int, int) "wraps" negative x to `Width - x` (sic, out of range for x < 0).
// Sampling functions are const and thread-safe (the C# keeps its scratch values in fields; only the GetPixelColor32
// bpp-2 quirk reads such a field and colour sampling is not ported yet).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ksp {

class MapSO {
public:
    // MapSO.cs:7
    struct HeightAlpha {
        float height = 0.0f;
        float alpha = 0.0f;
        HeightAlpha() = default;
        HeightAlpha(float h, float a) : height(h), alpha(a) {}
        static HeightAlpha Lerp(const HeightAlpha& a, const HeightAlpha& b, float dt);  // MapSO.cs:25
    };

    // MapSO.cs:31
    enum class MapDepth : int { Greyscale = 1, HeightAlpha = 2, RGB = 3, RGBA = 4 };

    static constexpr float Byte2Float = 0.003921569f;  // MapSO.cs:61
    static constexpr float Float2Byte = 255.0f;        // MapSO.cs:63

    // Where the data came from (CLAUDE.md "Original game assets"): asset file, PathID, Unity object name, map key.
    struct Origin {
        std::string file;
        int64_t pathID = 0;
        std::string objectName;
        std::string key;
    };

    MapSO() = default;

    // MapSO.cs:163 CreateMap(MapDepth, string, int, int): zero-filled pixels
    void CreateMap(MapDepth depth, const std::string& name, int width, int height);
    // A deserialized MapSO: header fields and _data as stored in the asset. Throws std::invalid_argument when the byte
    // count is not width * height * bpp or rowWidth is not width * bpp.
    static MapSO FromSerialized(const std::string& name, int width, int height, int bpp, int rowWidth,
                                std::vector<uint8_t> data, Origin origin);
    // (no `Origin origin = {}` default arguments: Clang rejects them while the class is incomplete; overloads instead)
    static MapSO FromSerialized(const std::string& name, int width, int height, int bpp, int rowWidth,
                                std::vector<uint8_t> data);
    // Loads <path> (raw MapSO._data) and checks its size against the header.
    static MapSO LoadRaw(const std::string& path, const std::string& name, int width, int height, int bpp,
                         Origin origin);
    static MapSO LoadRaw(const std::string& path, const std::string& name, int width, int height, int bpp);

    bool IsCompiled() const { return isCompiled_; }
    const std::string& MapName() const { return name_; }
    int Width() const { return width_; }
    int Height() const { return height_; }
    int BitsPerPixel() const { return bpp_; }  // sic: bytes per pixel (C# property name)
    int RowWidth() const { return rowWidth_; }
    MapDepth Depth() const { return static_cast<MapDepth>(bpp_); }
    int Size() const { return static_cast<int>(data_.size()); }
    const std::vector<uint8_t>& Data() const { return data_; }
    std::vector<uint8_t>& MutableData() { return data_; }
    const Origin& GetOrigin() const { return origin_; }

    int PixelIndex(int x, int y) const { return x * bpp_ + y * rowWidth_; }  // MapSO.cs:439

    uint8_t GreyByte(int x, int y) const;                      // MapSO.cs:546
    float GreyFloat(int x, int y) const;                       // MapSO.cs:562
    uint8_t GetPixelByte(int x, int y) const;                  // MapSO.cs:567
    float GetPixelFloat(int x, int y) const;                   // MapSO.cs:588
    HeightAlpha GetPixelHeightAlpha(int x, int y) const;       // MapSO.cs:641
    float GetPixelFloat(float x, float y) const;               // MapSO.cs:668
    HeightAlpha GetPixelHeightAlpha(float x, float y) const;   // MapSO.cs:678
    float GetPixelFloat(double x, double y) const;             // MapSO.cs:696
    HeightAlpha GetPixelHeightAlpha(double x, double y) const; // MapSO.cs:706
    void SetPixel(int x, int y, uint8_t b);                    // MapSO.cs:712

    // MapSO.cs:758 / :780 (the values the C# stores in minX/maxX/minY/maxY/midX/midY)
    struct BilinearCoords {
        int minX = 0, maxX = 0, minY = 0, maxY = 0;
        float midX = 0.0f, midY = 0.0f;
    };
    BilinearCoords ConstructBilinearCoords(float x, float y) const;
    BilinearCoords ConstructBilinearCoords(double x, double y) const;

private:
    bool isCompiled_ = false;
    std::string name_;
    int width_ = 0;
    int height_ = 0;
    int bpp_ = 0;
    int rowWidth_ = 0;
    std::vector<uint8_t> data_;
    Origin origin_;
};

}  // namespace ksp

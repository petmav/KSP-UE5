// WP-11: port of Assembly-CSharp/MapSO.cs (height/alpha sampling paths). Managed float code follows the Mono rule
// (ksp/dotnet/mono_float.hpp): every float local/field/argument/return is an f32store of a double expression.
#include "ksp/terrain/map_so.hpp"

#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <utility>

#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

namespace {

// _data[i] -> Byte2Float * (float)(int)_data[i]   (a float expression passed/returned/stored: one f32store)
inline float ByteToFloat(uint8_t b) { return f32store(fx(MapSO::Byte2Float) * static_cast<double>(b)); }

// C# out-of-range index on a byte[]: IndexOutOfRangeException (std::out_of_range per README "Errors")
inline uint8_t At(const std::vector<uint8_t>& data, int index) {
    if (index < 0 || static_cast<size_t>(index) >= data.size()) throw std::out_of_range("Index was outside the bounds of the array.");
    return data[static_cast<size_t>(index)];
}

}  // namespace

// MapSO.cs:25 new HeightAlpha(a.height + (b.height - a.height) * dt, a.alpha + (b.alpha - a.alpha) * dt)
MapSO::HeightAlpha MapSO::HeightAlpha::Lerp(const HeightAlpha& a, const HeightAlpha& b, float dt) {
    return HeightAlpha(f32store(fx(a.height) + (fx(b.height) - fx(a.height)) * fx(dt)),
                       f32store(fx(a.alpha) + (fx(b.alpha) - fx(a.alpha)) * fx(dt)));
}

// MapSO.cs:163
void MapSO::CreateMap(MapDepth depth, const std::string& name, int width, int height) {
    name_ = name;
    width_ = width;
    height_ = height;
    bpp_ = static_cast<int>(depth);
    rowWidth_ = width_ * bpp_;
    data_.assign(static_cast<size_t>(width_) * static_cast<size_t>(height_) * static_cast<size_t>(bpp_), 0);
    isCompiled_ = true;
}

MapSO MapSO::FromSerialized(const std::string& name, int width, int height, int bpp, int rowWidth,
                            std::vector<uint8_t> data, Origin origin) {
    if (width <= 0 || height <= 0 || bpp <= 0 || rowWidth != width * bpp ||
        data.size() != static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(bpp))
        throw std::invalid_argument("MapSO '" + name + "': data size " + std::to_string(data.size()) + " does not match " +
                                    std::to_string(width) + "x" + std::to_string(height) + "x" + std::to_string(bpp));
    MapSO m;
    m.name_ = name;
    m.width_ = width;
    m.height_ = height;
    m.bpp_ = bpp;
    m.rowWidth_ = rowWidth;
    m.data_ = std::move(data);
    m.origin_ = std::move(origin);
    m.isCompiled_ = true;
    return m;
}

MapSO MapSO::LoadRaw(const std::string& path, const std::string& name, int width, int height, int bpp, Origin origin) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("MapSO '" + name + "': cannot open " + path);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return FromSerialized(name, width, height, bpp, width * bpp, std::move(data), std::move(origin));
}

// MapSO.cs:546
uint8_t MapSO::GreyByte(int x, int y) const { return At(data_, PixelIndex(x, y)); }

// MapSO.cs:562
float MapSO::GreyFloat(int x, int y) const { return ByteToFloat(At(data_, PixelIndex(x, y))); }

// MapSO.cs:567 (sic: x < 0 becomes Width - x)
uint8_t MapSO::GetPixelByte(int x, int y) const {
    if (x < 0) {
        x = Width() - x;
    } else if (x >= Width()) {
        x -= Width();
    }
    if (y < 0) {
        y = Height() - y;
    } else if (y >= Height()) {
        y -= Height();
    }
    return At(data_, PixelIndex(x, y));
}

// MapSO.cs:588: retVal = 0f; retVal += (int)_data[index + itr] per channel; retVal /= _bpp; retVal *= Byte2Float
float MapSO::GetPixelFloat(int x, int y) const {
    float retVal = 0.0f;
    const int index = PixelIndex(x, y);
    for (int itr = 0; itr < bpp_; itr++) retVal = f32store(fx(retVal) + static_cast<double>(At(data_, index + itr)));
    retVal = f32store(fx(retVal) / static_cast<double>(static_cast<float>(bpp_)));
    retVal = f32store(fx(retVal) * fx(Byte2Float));
    return retVal;
}

// MapSO.cs:641
MapSO::HeightAlpha MapSO::GetPixelHeightAlpha(int x, int y) const {
    const int index = PixelIndex(x, y);
    if (bpp_ == 2) return HeightAlpha(ByteToFloat(At(data_, index)), ByteToFloat(At(data_, index + 1)));
    if (bpp_ == 4) return HeightAlpha(ByteToFloat(At(data_, index)), ByteToFloat(At(data_, index + 3)));
    return HeightAlpha(ByteToFloat(At(data_, index)), 1.0f);
}

// MapSO.cs:668
float MapSO::GetPixelFloat(float x, float y) const {
    const BilinearCoords c = ConstructBilinearCoords(x, y);
    if (bpp_ == 1)
        return Mathf::Lerp(Mathf::Lerp(GreyFloat(c.minX, c.minY), GreyFloat(c.maxX, c.minY), c.midX),
                           Mathf::Lerp(GreyFloat(c.minX, c.maxY), GreyFloat(c.maxX, c.maxY), c.midX), c.midY);
    return Mathf::Lerp(Mathf::Lerp(GetPixelFloat(c.minX, c.minY), GetPixelFloat(c.maxX, c.minY), c.midX),
                       Mathf::Lerp(GetPixelFloat(c.minX, c.maxY), GetPixelFloat(c.maxX, c.maxY), c.midX), c.midY);
}

// MapSO.cs:678
MapSO::HeightAlpha MapSO::GetPixelHeightAlpha(float x, float y) const {
    const BilinearCoords c = ConstructBilinearCoords(x, y);
    return HeightAlpha::Lerp(HeightAlpha::Lerp(GetPixelHeightAlpha(c.minX, c.minY), GetPixelHeightAlpha(c.maxX, c.minY), c.midX),
                             HeightAlpha::Lerp(GetPixelHeightAlpha(c.minX, c.maxY), GetPixelHeightAlpha(c.maxX, c.maxY), c.midX),
                             c.midY);
}

// MapSO.cs:696
float MapSO::GetPixelFloat(double x, double y) const {
    const BilinearCoords c = ConstructBilinearCoords(x, y);
    if (bpp_ == 1)
        return Mathf::Lerp(Mathf::Lerp(GreyFloat(c.minX, c.minY), GreyFloat(c.maxX, c.minY), c.midX),
                           Mathf::Lerp(GreyFloat(c.minX, c.maxY), GreyFloat(c.maxX, c.maxY), c.midX), c.midY);
    return Mathf::Lerp(Mathf::Lerp(GetPixelFloat(c.minX, c.minY), GetPixelFloat(c.maxX, c.minY), c.midX),
                       Mathf::Lerp(GetPixelFloat(c.minX, c.maxY), GetPixelFloat(c.maxX, c.maxY), c.midX), c.midY);
}

// MapSO.cs:706
MapSO::HeightAlpha MapSO::GetPixelHeightAlpha(double x, double y) const {
    const BilinearCoords c = ConstructBilinearCoords(x, y);
    return HeightAlpha::Lerp(HeightAlpha::Lerp(GetPixelHeightAlpha(c.minX, c.minY), GetPixelHeightAlpha(c.maxX, c.minY), c.midX),
                             HeightAlpha::Lerp(GetPixelHeightAlpha(c.minX, c.maxY), GetPixelHeightAlpha(c.maxX, c.maxY), c.midX),
                             c.midY);
}

// MapSO.cs:712 (sic: x < 0 becomes Width - x)
void MapSO::SetPixel(int x, int y, uint8_t b) {
    if (x < 0) {
        x = Width() - x;
    } else if (x >= Width()) {
        x -= Width();
    }
    if (y < 0) {
        y = Height() - y;
    } else if (y >= Height()) {
        y -= Height();
    }
    const int i = PixelIndex(x, y);
    if (i < 0 || static_cast<size_t>(i) >= data_.size()) throw std::out_of_range("Index was outside the bounds of the array.");
    data_[static_cast<size_t>(i)] = b;
}

// MapSO.cs:758 (managed float: Mathf.Abs/Floor return float; centerX/midX are float fields)
MapSO::BilinearCoords MapSO::ConstructBilinearCoords(float x, float y) const {
    BilinearCoords c;
    x = Mathf::Abs(f32store(fx(x) - fx(Mathf::Floor(x))));
    y = Mathf::Abs(f32store(fx(y) - fx(Mathf::Floor(y))));
    const float centerX = f32store(fx(x) * static_cast<double>(static_cast<float>(width_)));
    c.minX = Mathf::FloorToInt(centerX);
    c.maxX = Mathf::CeilToInt(centerX);
    c.midX = f32store(fx(centerX) - static_cast<double>(static_cast<float>(c.minX)));
    if (c.maxX == width_) c.maxX = 0;
    const float centerY = f32store(fx(y) * static_cast<double>(static_cast<float>(height_)));
    c.minY = Mathf::FloorToInt(centerY);
    c.maxY = Mathf::CeilToInt(centerY);
    c.midY = f32store(fx(centerY) - static_cast<double>(static_cast<float>(c.minY)));
    if (c.maxY == height_) c.maxY = 0;
    return c;
}

// MapSO.cs:780 (System.Math.Abs/Floor/Ceiling, (int) = conv.i4)
MapSO::BilinearCoords MapSO::ConstructBilinearCoords(double x, double y) const {
    BilinearCoords c;
    x = std::fabs(x - dotnet::Math::Floor(x));
    y = std::fabs(y - dotnet::Math::Floor(y));
    const double centerXD = x * static_cast<double>(width_);
    c.minX = dotnet::ConvToInt32(dotnet::Math::Floor(centerXD));
    c.maxX = dotnet::ConvToInt32(dotnet::Math::Ceiling(centerXD));
    c.midX = f32store(centerXD - static_cast<double>(c.minX));
    if (c.maxX == width_) c.maxX = 0;
    const double centerYD = y * static_cast<double>(height_);
    c.minY = dotnet::ConvToInt32(dotnet::Math::Floor(centerYD));
    c.maxY = dotnet::ConvToInt32(dotnet::Math::Ceiling(centerYD));
    c.midY = f32store(centerYD - static_cast<double>(c.minY));
    if (c.maxY == height_) c.maxY = 0;
    return c;
}

}  // namespace ksp

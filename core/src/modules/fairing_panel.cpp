// WP-26: ProceduralFairings/FairingXSection.cs, FairingPanel.cs, MeshPoint.cs (see fairing_panel.hpp for what is not ported).
// Managed float code follows the Mono rule (fx / f32store); Quaternion.AngleAxis / FromToRotation are Unity-native (UnityNative::).
#include "ksp/modules/fairing_panel.hpp"

#include <cmath>

#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/modules/module_procedural_fairing.hpp"
#include "ksp/unity/quaternion_native.hpp"

namespace ksp::ProceduralFairings {

namespace {
constexpr float kPiF = 3.14159274f;  // (float)Math.PI
}

// ---------------------------------------------------------------- FairingXSection

// FairingXSection.cs:44
FairingXSection::FairingXSection(std::shared_ptr<FairingXSection> from, std::shared_ptr<FairingXSection> to)
    : lerp1_(std::move(from)), lerp2_(std::move(to)), isLerp_(true) {}

// FairingXSection.cs:64
bool FairingXSection::AddAttachedFlag(int panelIndex, uint32_t flagPartID) {
    if (fairingPanelFlagsNull) {
        fairingPanelFlags.clear();
        fairingPanelFlagsNull = false;
    }
    bool result = false;
    for (size_t i = 0; i < fairingPanelFlags.size(); i++) {
        if (fairingPanelFlags[i].panelIndex == panelIndex) {
            result = true;
            auto& ids = fairingPanelFlags[i].attachedFlagsPartIds;  // AddUnique
            bool found = false;
            for (uint32_t v : ids) found = found || v == flagPartID;
            if (!found) ids.push_back(flagPartID);
            break;
        }
    }
    return result;
}

// FairingXSection.cs:83
void FairingXSection::AddNewFairingPanel(int panelIndex) {
    if (fairingPanelFlagsNull) {
        fairingPanelFlags.clear();
        fairingPanelFlagsNull = false;
    } else {
        for (size_t i = 0; i < fairingPanelFlags.size(); i++)
            if (fairingPanelFlags[i].panelIndex == panelIndex) return;
    }
    FairingPanelFlags f;
    f.panelIndex = panelIndex;
    fairingPanelFlags.push_back(std::move(f));
}

// FairingXSection.cs:104
int FairingXSection::GetPanelIndexFromFlag(uint32_t FlagID, uint32_t placementID) {
    if (fairingPanelFlagsNull) return -1;
    auto contains = [](const std::vector<uint32_t>& v, uint32_t x) {
        for (uint32_t e : v)
            if (e == x) return true;
        return false;
    };
    for (size_t i = 0; i < fairingPanelFlags.size(); i++) {
        const auto& ids = fairingPanelFlags[i].attachedFlagsPartIds;
        if (contains(ids, FlagID) || contains(ids, placementID)) return fairingPanelFlags[i].panelIndex;
    }
    return -1;
}

// FairingXSection.cs:127
void FairingXSection::RemoveAttachedFlag(uint32_t FlagID) {
    if (fairingPanelFlagsNull) return;
    for (auto& f : fairingPanelFlags) {
        auto& ids = f.attachedFlagsPartIds;
        for (size_t k = 0; k < ids.size(); k++)
            if (ids[k] == FlagID) {
                ids.erase(ids.begin() + static_cast<std::ptrdiff_t>(k));
                break;
            }
    }
}

// FairingXSection.cs:138
void FairingXSection::Load(const ConfigNode& node) {
    if (node.HasValue("h")) h = dotnet::ParseSingle(*node.GetValuePtr("h"));
    if (node.HasValue("r")) r = dotnet::ParseSingle(*node.GetValuePtr("r"));
    fairingPanelFlags.clear();
    fairingPanelFlagsNull = false;
    std::vector<ConfigNode*> nodes = node.GetNodes("ATTACHEDFLAG");
    if (nodes.empty()) return;
    for (ConfigNode* n : nodes) {
        std::string value;
        int32_t result = 0;
        if (const std::string* p = n->GetValuePtr("panelIndex")) value = *p;
        if (!dotnet::TryParseInt32(value, result)) continue;
        FairingPanelFlags item;
        item.panelIndex = result;
        fairingPanelFlags.push_back(std::move(item));
        std::string value2;
        const std::string* p2 = n->GetValuePtr("attachedFlagPartIDs");
        if (p2 == nullptr) continue;
        value2 = *p2;
        for (const std::string& piece : dotnet::Split(value2, ",", false)) {
            uint32_t r2 = 0;
            if (dotnet::TryParseUInt32(piece, r2)) fairingPanelFlags.back().attachedFlagsPartIds.push_back(r2);
        }
    }
}

// FairingXSection.cs:183
void FairingXSection::Save(ConfigNode& node) const {
    node.AddValue("h", h);
    node.AddValue("r", r);
    if (fairingPanelFlagsNull) return;
    for (const auto& f : fairingPanelFlags) {
        ConfigNode* configNode = node.AddNode("ATTACHEDFLAG");
        std::string text;
        bool flag = false;
        for (uint32_t id : f.attachedFlagsPartIds) {
            if (flag) text += ",";
            text += std::to_string(id);
            flag = true;
        }
        configNode->AddValue("attachedFlagPartIDs", text);
        configNode->AddValue("panelIndex", static_cast<int32_t>(f.panelIndex));
    }
}

// FairingXSection.cs:210 (Color.Lerp: t clamped, components a + (b - a) * t)
void FairingXSection::UpdateLerp(float t, float y) {
    if (!isLerp_) return;
    h = Mathf::Lerp(lerp1_->h, lerp2_->h, t);
    r = f32store(fx(Mathf::Lerp(lerp1_->r, lerp2_->r, t)) + fx(y));
    const float tc = Mathf::Clamp01(t);
    auto lc = [&](float a, float b) { return f32store(fx(a) + (fx(b) - fx(a)) * fx(tc)); };
    color = Color(lc(lerp1_->color.r, lerp2_->color.r), lc(lerp1_->color.g, lerp2_->color.g), lc(lerp1_->color.b, lerp2_->color.b),
                  lc(lerp1_->color.a, lerp2_->color.a));
}

// FairingXSection.cs:220
float FairingXSection::GetSlopeAngle(const FairingXSection& from, const FairingXSection& to) {
    return Mathf::Atan(f32store(fx(f32store(fx(to.h) - fx(from.h))) / fx(f32store(fx(from.r) - fx(to.r)))));
}

// FairingXSection.cs:276 (float.CompareTo)
int FairingXSection::CompareTo(const FairingXSection& b) const {
    if (h < b.h) return -1;
    if (h > b.h) return 1;
    if (h == b.h) return 0;
    if (std::isnan(h)) return std::isnan(b.h) ? 0 : -1;
    return 1;
}

// ---------------------------------------------------------------- MeshPoint

// MeshPoint.cs:19
void MeshPoint::Clear() {
    vert = Vector3f::zero();
    normal = Vector3f::zero();
    uv1 = Vector2f::zero();
    uv2 = Vector2f::zero();
    color = Color::clear();
    index = 0;
}

// ---------------------------------------------------------------- FairingPanel

// FairingPanel.cs:603
void FairingPanel::LazyInitMeshArray(MeshPointArray& mArray, int length) {
    if (mArray.empty()) {
        mArray.resize(static_cast<size_t>(length));
        for (int n = length; n-- > 0;) mArray[static_cast<size_t>(n)] = std::make_shared<MeshPoint>();
    } else {
        for (int n = length; n-- > 0;) mArray.at(static_cast<size_t>(n))->Clear();
    }
}

// FairingPanel.cs:93
FairingPanel::FairingPanel(const std::vector<std::shared_ptr<FairingXSection>>& xSections, float arcFrom, float arcTo, ModuleProceduralFairing* host) {
    if (xSections.size() < 2) {
        log::Error("[FairingPanel]: Cannot create panel with less than two cross sections!");
        return;
    }
    arcFrom_ = arcFrom;
    arcTo_ = arcTo;
    pivotOffset = Vector3f::zero();
    host_ = host;
    // nSides = (int)((float)host.nSides * ((arcTo - arcFrom) / 360f)): the cast truncates the unrounded value
    nSides_ = dotnet::ConvToInt32(fx(static_cast<float>(host->nSides())) * ((fx(arcTo) - fx(arcFrom)) / 360.0));
    Arcs_.assign(xSections.size(), nullptr);
    pCount_ = 0;
    for (size_t i = 0; i < xSections.size(); i++) {
        auto arc = std::make_shared<MeshArc>(xSections[i], MeshPointArray{}, MeshPointArray{});
        if (!xSections[i]->isCap) {
            LazyInitMeshArray(arc->inner, nSides_);
            LazyInitMeshArray(arc->outer, nSides_);
            pCount_ += nSides_ * 2;
        } else {
            LazyInitMeshArray(arc->inner, 1);
            LazyInitMeshArray(arc->outer, 1);
            pCount_ += 2;
        }
        Arcs_[i] = arc;
    }
    LazyInitMeshArray(pts_, pCount_);
    ptsNull_ = false;
    tris_.clear();
    trisNull_ = false;
}

// FairingPanel.cs:138
bool FairingPanel::ContainsSection(const FairingXSection* sec) const {
    for (size_t n = Arcs_.size(); n-- > 0;)
        if (Arcs_[n]->xSection.get() == sec) return true;
    return false;
}

// FairingPanel.cs:152
std::vector<std::shared_ptr<FairingPanel>> FairingPanel::SetupPanelArray(int nSegments, std::vector<std::shared_ptr<FairingXSection>>& xSections,
                                                                         ModuleProceduralFairing* host, float capRadius) {
    std::vector<std::shared_ptr<FairingPanel>> list;
    const float num = f32store(360.0 / fx(static_cast<float>(nSegments)));
    int num2 = 0;
    for (int i = 0, num3 = static_cast<int>(xSections.size()) - 1; i < num3; i++) {
        std::shared_ptr<FairingXSection> fairingXSection = xSections[static_cast<size_t>(i)];
        std::shared_ptr<FairingXSection> fairingXSection2 = xSections[static_cast<size_t>(i) + 1];
        auto fairingXSection3 = std::make_shared<FairingXSection>(fairingXSection, fairingXSection2);
        auto fairingXSection4 = std::make_shared<FairingXSection>(fairingXSection2, fairingXSection3);
        for (int j = 0; j < nSegments; j++) {
            const float num4 = f32store(fx(num) * fx(static_cast<float>(j)));
            const float num5 = f32store(fx(num) * fx(static_cast<float>(j)) + fx(num));
            auto fairingPanel = std::make_shared<FairingPanel>(
                std::vector<std::shared_ptr<FairingXSection>>{fairingXSection, fairingXSection3, fairingXSection4, fairingXSection2}, num4, num5, host);
            fairingPanel->bottom = fairingXSection;
            fairingPanel->top = fairingXSection2;
            if (i > 0) {
                list.at(static_cast<size_t>(num2 - nSegments))->topNeighbour_ = fairingPanel.get();
                fairingPanel->bottomNeighbour_ = list.at(static_cast<size_t>(num2 - nSegments)).get();
            }
            fairingPanel->panelIndex = static_cast<int>(list.size());
            xSections[static_cast<size_t>(i)]->AddNewFairingPanel(fairingPanel->panelIndex);
            fairingPanel->FetchExistingFlagReferences(xSections, i);
            list.push_back(fairingPanel);
            num2++;
        }
    }
    const auto& lastXs = xSections.back();
    if (lastXs->isLast && lastXs->r == capRadius) {
        std::shared_ptr<FairingXSection> fairingXSection = xSections.back();
        auto fairingXSection2 = std::make_shared<FairingXSection>();
        fairingXSection2->r = 0.0f;
        fairingXSection2->h = f32store(fx(fairingXSection->h) +
                                       fx(fairingXSection->r) * fx(Mathf::Tan(FairingXSection::GetSlopeAngle(*xSections[xSections.size() - 2], *fairingXSection))) *
                                           fx(host->noseTip()));
        fairingXSection2->isCap = true;
        for (int k = 0; k < nSegments; k++) {
            FairingPanel* fairingPanel = list.at(list.size() - static_cast<size_t>(nSegments)).get();
            const float num4 = f32store(fx(num) * fx(static_cast<float>(k)));
            const float num5 = f32store(fx(num) * fx(static_cast<float>(k)) + fx(num));
            auto fairingPanel2 = std::make_shared<FairingPanel>(std::vector<std::shared_ptr<FairingXSection>>{fairingXSection, fairingXSection2}, num4, num5, host);
            fairingPanel->topNeighbour_ = fairingPanel2.get();
            fairingPanel->isLast = true;
            fairingPanel2->bottomNeighbour_ = fairingPanel;
            fairingPanel2->bottom = fairingXSection;
            fairingPanel2->top = fairingXSection2;
            fairingPanel2->isCap = true;
            fairingPanel2->panelIndex = static_cast<int>(list.size());
            fairingPanel2->persistentXSection = xSections.back();
            xSections.back()->AddNewFairingPanel(fairingPanel2->panelIndex);
            fairingPanel2->FetchExistingFlagReferences(xSections, static_cast<int>(xSections.size()) - 1);
            list.push_back(fairingPanel2);
        }
    }
    return list;
}

// FairingPanel.cs:212 (only the bookkeeping that has an effect: the C# creates AttachedFlagParts objects it never stores)
void FairingPanel::FetchExistingFlagReferences(const std::vector<std::shared_ptr<FairingXSection>>& xSections, int index) {
    const FairingXSection& xs = *xSections[static_cast<size_t>(index)];
    if (xs.fairingPanelFlagsNull || xs.fairingPanelFlags.empty()) return;
    for (const auto& f : xs.fairingPanelFlags) {
        if (f.panelIndex == panelIndex) {
            if (attachedFlagPartsNull) {
                attachedFlagParts.clear();
                attachedFlagPartsNull = false;
            }
        }
    }
}

// FairingPanel.cs:255
void FairingPanel::CleanUp() {
    pts_.clear();
    ptsNull_ = true;
    tris_.clear();
    trisNull_ = true;
}

// FairingPanel.cs:261
void FairingPanel::UpdateInterpolations() {
    const float num = (topNeighbour_ == nullptr) ? 0.0f : f32store(fx(GetSlope()) - fx(topNeighbour_->GetSlope()));
    const float num2 = (bottomNeighbour_ == nullptr) ? 0.0f : f32store(fx(bottomNeighbour_->GetSlope()) - fx(GetSlope()));
    Arcs_.at(1)->xSection->UpdateLerp(host_->edgeSlide(), f32store(fx(num2) * fx(host_->edgeWarp())));
    Arcs_.at(2)->xSection->UpdateLerp(host_->edgeSlide(), f32store(fx(num) * fx(host_->edgeWarp())));
}

// FairingPanel.cs:269
float FairingPanel::GetSlope() const {
    return f32store(fx(f32store(fx(Arcs_[0]->xSection->r) - fx(Arcs_.back()->xSection->r))) /
                    fx(f32store(fx(Arcs_[0]->xSection->h) - fx(Arcs_.back()->xSection->h))));
}

// FairingPanel.cs:274 (`go == null` cannot happen: the model object does not exist here; Spawn is the host's)
void FairingPanel::BuildMesh(bool triangulate, const FairingPanel* prevPanel) {
    if (Arcs_.size() > 2) UpdateInterpolations();
    LazyInitMeshArray(pts_, pCount_);
    ptsNull_ = false;
    if (triangulate) tris_.clear();
    const MeshArc* meshArc = nullptr;
    if (prevPanel != nullptr && !prevPanel->Arcs_.empty()) meshArc = prevPanel->Arcs_.back().get();
    const Vector3f forward = Vector3f::forward();
    bool flag = false;
    const Vector3f axis = host_->axis();
    const float nArcs = host_->nArcs();
    const int numArcs = static_cast<int>(Arcs_.size());
    auto prevHOuter = [&]() -> const std::vector<float>* { return (meshArc != nullptr && !meshArc->hOffsetOuter.empty()) ? &meshArc->hOffsetOuter : nullptr; };
    auto prevHInner = [&]() -> const std::vector<float>* { return (meshArc != nullptr && !meshArc->hOffsetInner.empty()) ? &meshArc->hOffsetInner : nullptr; };
    auto prevOuter = [&]() -> const MeshPointArray* { return (meshArc != nullptr && !meshArc->outer.empty()) ? &meshArc->outer : nullptr; };
    auto prevInner = [&]() -> const MeshPointArray* { return (meshArc != nullptr && !meshArc->inner.empty()) ? &meshArc->inner : nullptr; };
    for (int i = 0; i < numArcs; i++) {
        if (i == 0) {
            MeshArc& meshArc2 = *Arcs_[0];
            LazyInitMeshArray(meshArc2.outer, nSides_);
            LazyInitMeshArray(meshArc2.inner, nSides_);
            SetArcPivot(meshArc2);
            continue;
        }
        MeshArc& meshArc3 = *Arcs_[static_cast<size_t>(i)];
        MeshArc& meshArc2 = *Arcs_[static_cast<size_t>(i) - 1];
        if (!meshArc3.xSection->isCap) {
            LazyInitMeshArray(meshArc3.outer, nSides_);
            LazyInitMeshArray(meshArc3.inner, nSides_);
        } else {
            LazyInitMeshArray(meshArc3.outer, 1);
            LazyInitMeshArray(meshArc3.inner, 1);
        }
        SetArcPivot(meshArc3);
        GetPanelSlope(meshArc2, meshArc3, slope_);
        {
            const float sl = f32store(fx(f32store(fx(Arcs_[0]->xSection->r) - fx(Arcs_.back()->xSection->r))) /
                                      fx(f32store(fx(Arcs_[0]->xSection->h) - fx(Arcs_.back()->xSection->h))));
            flag = Mathf::Abs(sl) > host_->aberrantNormalLimit();
        }
        float h;
        if (i == 1) {
            h = meshArc2.xSection->h;
            if (meshArc3.xSection->isCap) {
                if (ModuleProceduralFairing::loadedSceneIsEditorOrFlight()) {
                    meshArc2.hOffsetOuter = SetArcPoints(meshArc2.pivot, axis, nArcs, Vector3f::forward(), forward, slope_, arcFrom_, arcTo_,
                                                         meshArc2.xSection->r, 0.0f, 1.0f, 0.0f, 1.0f, meshArc2.xSection->color, meshArc2.outer, nullptr, nullptr);
                    meshArc2.hOffsetInner = SetArcPoints(meshArc2.pivot, axis, nArcs, Vector3f::forward(), -forward, slope_, arcFrom_, arcTo_,
                                                         f32store(fx(meshArc2.xSection->r) - fx(wallThickness_)), 0.0f, 1.0f, 0.0f, 1.0f,
                                                         meshArc2.xSection->color, meshArc2.inner, nullptr, nullptr);
                }
            } else {
                meshArc2.hOffsetOuter = SetArcPoints(meshArc2.pivot, axis, nArcs, Vector3f::forward(), forward, slope_, arcFrom_, arcTo_,
                                                     meshArc2.xSection->r, h, 0.5f, 0.0f, 0.5f, meshArc2.xSection->color, meshArc2.outer, prevHOuter(), prevOuter());
                meshArc2.hOffsetInner = SetArcPoints(meshArc2.pivot, axis, nArcs, Vector3f::forward(), -forward, slope_, arcFrom_, arcTo_,
                                                     f32store(fx(meshArc2.xSection->r) - fx(wallThickness_)), h, 0.0f, 0.5f, 0.5f, meshArc2.xSection->color,
                                                     meshArc2.inner, prevHInner(), prevInner());
            }
            SetPanelMeshPoints(pts_, meshArc2.outer, nSides_ * 2 * (i - 1), flag);
            SetPanelMeshPoints(pts_, meshArc2.inner, nSides_ * 2 * (i - 1) + nSides_, flag);
        }
        h = meshArc3.xSection->h;
        if (!meshArc3.xSection->isCap) {
            meshArc3.hOffsetOuter = SetArcPoints(meshArc3.pivot, axis, nArcs, Vector3f::forward(), forward, slope_, arcFrom_, arcTo_, meshArc3.xSection->r,
                                                 h, 0.5f, 0.0f, 0.5f, meshArc3.xSection->color, meshArc3.outer,
                                                 meshArc2.hOffsetOuter.empty() ? nullptr : &meshArc2.hOffsetOuter, meshArc2.outer.empty() ? nullptr : &meshArc2.outer);
            meshArc3.hOffsetInner = SetArcPoints(meshArc3.pivot, axis, nArcs, Vector3f::forward(), -forward, slope_, arcFrom_, arcTo_,
                                                 f32store(fx(meshArc3.xSection->r) - fx(wallThickness_)), h, 0.0f, 0.5f, 0.5f, meshArc3.xSection->color,
                                                 meshArc3.inner, meshArc2.hOffsetInner.empty() ? nullptr : &meshArc2.hOffsetInner,
                                                 meshArc2.inner.empty() ? nullptr : &meshArc2.inner);
            SetPanelMeshPoints(pts_, meshArc3.outer, nSides_ * 2 * (i - 1) + nSides_ * 2, flag);
            SetPanelMeshPoints(pts_, meshArc3.inner, nSides_ * 2 * (i - 1) + nSides_ * 3, flag);
            if (triangulate)
                TriangulatePanelMesh(meshArc2.outer, meshArc3.outer, meshArc2.inner, meshArc3.inner, nSides_, i == numArcs - 1, i == 1, tris_);
        } else {
            SetNoseConePoints(meshArc3.pivot, axis, Vector3f::up(), meshArc3.xSection->r, 0.5f, meshArc3.outer);
            SetNoseConePoints(meshArc3.pivot, axis, Vector3f::down(), meshArc3.xSection->r, 0.5f, meshArc3.inner);
            SetPanelMeshPoints(pts_, meshArc3.outer, nSides_ * 2 * i, false);
            SetPanelMeshPoints(pts_, meshArc3.inner, nSides_ * 2 * i + static_cast<int>(meshArc3.outer.size()), false);
            if (triangulate) TriangulateConePanel(meshArc2.outer, meshArc3.outer, meshArc2.inner, meshArc3.inner, false, i == 1, tris_);
        }
    }
    pivotOffset = GetBarycenter(pts_);
    explodedPos = pivotOffset + (pivotOffset - GetMidPoint()).normalized() * Mathf::Max(Arcs_[0]->xSection->r, Arcs_.back()->xSection->r);
    // AssignMesh (:386): vertices (offset by -pivotOffset), normals, uv, uv2, colors, triangles
    mesh.vertices.clear();
    mesh.normals.clear();
    mesh.uv.clear();
    mesh.uv2.clear();
    mesh.colors.clear();
    const Vector3f offset = -pivotOffset;
    mesh.vertices.resize(pts_.size());
    mesh.normals.resize(pts_.size());
    mesh.uv.resize(pts_.size());
    mesh.uv2.resize(pts_.size());
    mesh.colors.resize(pts_.size());
    for (size_t n = pts_.size(); n-- > 0;) {
        mesh.vertices[n] = pts_[n]->vert + offset;
        mesh.normals[n] = pts_[n]->normal;
        mesh.uv[n] = pts_[n]->uv1;
        mesh.uv2[n] = pts_[n]->uv2;
        mesh.colors[n] = pts_[n]->color;
    }
    if (triangulate) mesh.triangles = tris_;
}

// FairingPanel.cs:375
Vector3f FairingPanel::GetBarycenter(const MeshPointArray& pts) {
    Vector3f zero = Vector3f::zero();
    for (size_t n = pts.size(); n-- > 0;) zero += pts[n]->vert;
    return zero / static_cast<float>(pts.size());
}

// FairingPanel.cs:400
void FairingPanel::SetArcPivot(MeshArc& arc) { arc.pivot = host_->pivot() + host_->axis() * arc.xSection->h; }

// FairingPanel.cs:405
void FairingPanel::GetPanelSlope(const MeshArc& bottomArc, const MeshArc& topArc, Quaternionf& slope) {
    if (topArc.xSection->r != bottomArc.xSection->r) {
        slope = UnityNative::FromToRotation(host_->axis(), (topArc.pivot + Vector3f::forward() * topArc.xSection->r) -
                                                               (bottomArc.pivot + Vector3f::forward() * bottomArc.xSection->r));
    } else {
        slope = Quaternionf::identity();
    }
}

// FairingPanel.cs:417
float FairingPanel::GetPanelHeight() const { return f32store(fx(Arcs_.back()->xSection->h) - fx(Arcs_[0]->xSection->h)); }

// FairingPanel.cs:422
float FairingPanel::GetBaseHeight() const { return Arcs_[0]->xSection->h; }

// FairingPanel.cs:427
Vector3f FairingPanel::GetMidPoint() const {
    return host_->pivot() + host_->axis() * f32store(fx(GetBaseHeight()) + fx(GetPanelHeight()) * 0.5);
}

// FairingPanel.cs:432
std::vector<float> FairingPanel::SetArcPoints(const Vector3f& pivot, const Vector3f& axis, float /*nArcs*/, const Vector3f& v0, const Vector3f& n0,
                                              const Quaternionf& slope, float fromAngle, float toAngle, float radius, float h, float arcOffset,
                                              float uOffset, float uTiling, const Color& vColor, MeshPointArray& arcPoints,
                                              const std::vector<float>* prevH, const MeshPointArray* prevPoints) {
    std::vector<float> array(arcPoints.size());
    int num = static_cast<int>(arcPoints.size());
    while (num-- > 0) {
        const float num2 = Mathf::InverseLerp(0.0f, static_cast<float>(arcPoints.size()) - 1.0f, static_cast<float>(num));
        const float angle = Mathf::Lerp(fromAngle, toAngle, num2);
        MeshPoint& meshPoint = *arcPoints[static_cast<size_t>(num)];
        meshPoint.Clear();
        meshPoint.normal = UnityNative::AngleAxis(angle, axis) * (slope * n0);
        meshPoint.vert = pivot + (UnityNative::AngleAxis(angle, axis) * v0) * radius;
        if (prevPoints == nullptr) {
            array[static_cast<size_t>(num)] = h;
        } else {
            const float ph = prevH->at(static_cast<size_t>(num));
            const float hd = f32store(fx(h) - fx(ph));
            const float dist = Vector3f::Distance((*prevPoints)[static_cast<size_t>(num)]->vert, meshPoint.vert);
            array[static_cast<size_t>(num)] = f32store(fx(ph) + fx(dotnet::Math::Max(hd, dist)));
        }
        const float absArg = f32store(fx(arcOffset) - fx(num2) * fx(uTiling));
        meshPoint.uv1 = Vector2f{f32store(fx(Mathf::Abs(absArg)) + fx(uOffset)), f32store(fx(array[static_cast<size_t>(num)]) / 3.0)};
        meshPoint.color = vColor;
    }
    return array;
}

// FairingPanel.cs:451
int FairingPanel::SetPanelMeshPoints(MeshPointArray& pts, MeshPointArray& arc, int ptsOffset, bool aberrantNormals) {
    int num = 0;
    for (int i = 0, num2 = static_cast<int>(arc.size()); i < num2; i++) {
        num = i + ptsOffset;
        pts.at(static_cast<size_t>(num)) = arc[static_cast<size_t>(i)];
        pts[static_cast<size_t>(num)]->index = num;
        if (aberrantNormals && num < static_cast<int>(pts.size()) / 2) pts[static_cast<size_t>(num)]->normal *= -1.0f;
    }
    return num;
}

// FairingPanel.cs:468
void FairingPanel::TriangulatePanelMesh(const MeshPointArray& bottomArcOuter, const MeshPointArray& topArcOuter, const MeshPointArray& bottomArcInner,
                                        const MeshPointArray& topArcInner, int nSides, bool capTop, bool capBottom, std::vector<int>& tris) {
    auto ix = [](const MeshPointArray& a, int i) { return a.at(static_cast<size_t>(i))->index; };
    for (int i = 1; i < nSides; i++) {
        const int index = ix(bottomArcOuter, i - 1);
        const int index2 = ix(bottomArcOuter, i);
        const int index3 = ix(topArcOuter, i - 1);
        const int index4 = ix(topArcOuter, i);
        TriangulateQuad(index2, index4, index, index3, tris);
    }
    for (int j = 1; j < nSides; j++) {
        const int index = ix(bottomArcInner, j);
        const int index5 = ix(bottomArcInner, j - 1);
        const int index3 = ix(topArcInner, j);
        const int index4 = ix(topArcInner, j - 1);
        TriangulateQuad(index5, index4, index, index3, tris);
    }
    if (capBottom) {
        for (int k = 1; k < nSides; k++) {
            const int index6 = ix(bottomArcInner, k);
            const int index = ix(bottomArcInner, k - 1);
            const int index4 = ix(bottomArcOuter, k);
            const int index3 = ix(bottomArcOuter, k - 1);
            TriangulateQuad(index6, index4, index, index3, tris);
        }
    }
    if (capTop) {
        for (int l = 1; l < nSides; l++) {
            const int index7 = ix(topArcOuter, l);
            const int index = ix(topArcOuter, l - 1);
            const int index4 = ix(topArcInner, l);
            const int index3 = ix(topArcInner, l - 1);
            TriangulateQuad(index7, index4, index, index3, tris);
        }
    }
    TriangulateQuad(ix(bottomArcOuter, 0), ix(topArcOuter, 0), ix(bottomArcInner, 0), ix(topArcInner, 0), tris);
    TriangulateQuad(ix(bottomArcInner, nSides - 1), ix(topArcInner, nSides - 1), ix(bottomArcOuter, nSides - 1), ix(topArcOuter, nSides - 1), tris);
}

// FairingPanel.cs:512
void FairingPanel::TriangulateQuad(int v00, int v01, int v0, int v1, std::vector<int>& tris) {
    tris.push_back(v00);
    tris.push_back(v01);
    tris.push_back(v1);
    tris.push_back(v1);
    tris.push_back(v0);
    tris.push_back(v00);
}

// FairingPanel.cs:522
void FairingPanel::SetNoseConePoints(const Vector3f& pivot, const Vector3f& axis, const Vector3f& n0, float radius, float uOffset, MeshPointArray& pts) {
    MeshPoint& obj = *pts.at(0);
    obj.Clear();
    obj.vert = pivot + axis * radius;
    obj.normal = n0;
    obj.uv1 = Vector2f{uOffset, 1.0f};
}

// FairingPanel.cs:531
void FairingPanel::TriangulateConePanel(const MeshPointArray& bottomArcOuter, const MeshPointArray& topArcOuter, const MeshPointArray& bottomArcInner,
                                        const MeshPointArray& topArcInner, bool capTop, bool capBottom, std::vector<int>& tris) {
    auto ix = [](const MeshPointArray& a, size_t i) { return a.at(i)->index; };
    TriangulateTriangleMesh(bottomArcOuter, topArcOuter, static_cast<int>(bottomArcOuter.size()), static_cast<int>(topArcOuter.size()), tris, true);
    TriangulateTriangleMesh(bottomArcInner, topArcInner, static_cast<int>(bottomArcInner.size()), static_cast<int>(topArcInner.size()), tris, false);
    if (capBottom || capTop) {
        const int num = static_cast<int>(bottomArcOuter.size());
        if (capBottom) {
            for (int i = 1; i < num; i++) {
                const int index = ix(bottomArcInner, static_cast<size_t>(i));
                const int index2 = ix(bottomArcInner, static_cast<size_t>(i) - 1);
                const int index3 = ix(bottomArcOuter, static_cast<size_t>(i));
                const int index4 = ix(bottomArcOuter, static_cast<size_t>(i) - 1);
                TriangulateQuad(index, index3, index2, index4, tris);
            }
        }
        if (capTop) {
            for (int j = 1; j < num; j++) {
                const int index5 = ix(topArcOuter, static_cast<size_t>(j));
                const int index2 = ix(topArcOuter, static_cast<size_t>(j) - 1);
                const int index3 = ix(topArcInner, static_cast<size_t>(j));
                const int index4 = ix(topArcInner, static_cast<size_t>(j) - 1);
                TriangulateQuad(index5, index4, index2, index3, tris);
            }
        }
    }
    TriangulateQuad(ix(bottomArcOuter, 0), ix(topArcOuter, 0), ix(bottomArcInner, 0), ix(topArcInner, 0), tris);
    TriangulateQuad(ix(bottomArcInner, bottomArcInner.size() - 1), ix(topArcInner, topArcInner.size() - 1), ix(bottomArcOuter, bottomArcOuter.size() - 1),
                    ix(topArcOuter, topArcOuter.size() - 1), tris);
}

// FairingPanel.cs:565
void FairingPanel::TriangulateTriangleMesh(const MeshPointArray& arcOuter, const MeshPointArray& arcInner, int nSidesOuter, int nSidesInner,
                                           std::vector<int>& tris, bool flip) {
    const int num = nSidesOuter / nSidesInner;
    if (nSidesOuter % nSidesInner != 0) {  // (float)a % (float)b != 0f on small integers
        log::Error("[Procedural Fairings Mesh Util]: Cannot triangulate between a " + std::to_string(nSidesOuter) + "-sided arc and a " +
                   std::to_string(nSidesInner) + "-sided one. Vert Counts are not cleanly divisible.");
        return;
    }
    int num2 = 0;
    for (int i = 1; i < nSidesOuter; i++) {
        const int index = arcOuter.at(static_cast<size_t>(i) - 1)->index;
        const int index2 = arcOuter.at(static_cast<size_t>(i))->index;
        const int index3 = arcInner.at(static_cast<size_t>(num2))->index;
        TriangulateTri(index, index2, index3, tris, flip);
        if (i % num == 0) num2++;
    }
}

// FairingPanel.cs:587
void FairingPanel::TriangulateTri(int v00, int v0, int v1, std::vector<int>& tris, bool flip) {
    if (!flip) {
        tris.push_back(v00);
        tris.push_back(v1);
        tris.push_back(v0);
    } else {
        tris.push_back(v00);
        tris.push_back(v0);
        tris.push_back(v1);
    }
}

// FairingPanel.cs:624
float FairingPanel::GetArea() const {
    const float r = Arcs_[0]->xSection->r;
    const float r2 = Arcs_.back()->xSection->r;
    const float panelHeight = GetPanelHeight();
    const float num = Mathf::Sqrt(f32store(fx(Mathf::Pow(f32store(fx(r) - fx(r2)), 2.0f)) + fx(panelHeight) * fx(panelHeight)));
    const float num2 = f32store(fx(kPiF) * (fx(r) + fx(r2)) * fx(num));
    const float num3 = f32store((fx(arcTo_) - fx(arcFrom_)) / 360.0);
    return f32store(fx(num2) * fx(num3));
}

// FairingPanel.cs:830
void FairingPanel::RemoveFlag(uint32_t FlagID) {
    if (attachedFlagPartsNull) return;
    for (size_t i = 0; i < attachedFlagParts.size(); i++) {
        if (attachedFlagParts[i].flagVesselID == FlagID) {
            attachedFlagParts.erase(attachedFlagParts.begin() + static_cast<std::ptrdiff_t>(i));
            i--;
        }
    }
}

}  // namespace ksp::ProceduralFairings

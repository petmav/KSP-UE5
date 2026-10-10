// WP-42: LaunchSite.cs, KSCSiteNode.cs.
#include "ksp/space_center/launch_site.hpp"

#include <cfloat>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/space_center/pqs_city.hpp"
#include "ksp/space_center/psystem_setup.hpp"
#include "ksp/terrain/pqs.hpp"

namespace ksp {

using Localizer = KSP::Localization::Localizer;
using space_center::SceneGraph;

// LaunchSite.cs:30
void LaunchSite::SpawnPoint::Setup(LaunchSite* hostSite) { host = hostSite; }

// LaunchSite.cs:35
std::optional<Vector3d> LaunchSite::SpawnPoint::GetSpawnPointTransform() {
    if (!transformFound && SceneGraph::FindChild) {
        const auto found = SceneGraph::FindChild(host->pqsName, host->launchSiteTransformURL, spawnTransformURL);
        if (found) {
            transformFound = true;
            transformPosition = *found;
        }
    }
    if (!transformFound) {
        log::Info("[LaunchSite::SpawnPoint]: Cannot find a transform named '" + spawnTransformURL + "' on '" + name + "'");
        return std::nullopt;
    }
    return transformPosition;
}

// LaunchSite.cs:49
void LaunchSite::SpawnPoint::GetSpawnPointLatLonAlt(double& lat, double& lon, double& alt) const {
    lat = latitude;
    lon = longitude;
    alt = altitude;
}

// LaunchSite.cs:56 (the spawn point transform is re-found: the engine moves it when the site is re-oriented)
void LaunchSite::SpawnPoint::SetSpawnPointLatLonAlt() {
    if (SceneGraph::FindChild) {
        const auto found = SceneGraph::FindChild(host->pqsName, host->launchSiteTransformURL, spawnTransformURL);
        transformFound = found.has_value();
        if (found) transformPosition = *found;
    }
    if (!transformFound) {
        log::Info("[LaunchSite::SpawnPoint]: Cannot find a transform named '" + spawnTransformURL + "' on '" + name + "'");
        latlonaltSet = false;
    } else if (host->Body() != nullptr) {
        host->Body()->GetLatLonAlt(transformPosition, latitude, longitude, altitude);
        latlonaltSet = true;
    } else {
        log::Info("[LaunchSite::SpawnPoint]: (" + name + ") Unable to set Latitude, Longitude, Altitude");
        latlonaltSet = false;
    }
}

// LaunchSite.cs:131
CelestialBody* LaunchSite::Body() {
    if (body_ == nullptr) {
        if (isPQSCity()) body_ = pqsCity->body;
        if (isPQSCity2()) body_ = pqsCity2->body;
    }
    return body_;
}

// LaunchSite.cs:150
LaunchSite::LaunchSite(const std::string& name_, const std::string& pqsName_, const std::string& launchSiteName_, std::vector<SpawnPoint> spawnPoints_,
                       const std::string& launchSiteTransformURL_, EditorFacility editorFacility_)
    : name(name_), pqsName(pqsName_), launchSiteName(launchSiteName_), spawnPoints(std::move(spawnPoints_)), launchSiteTransformURL(launchSiteTransformURL_),
      editorFacility(editorFacility_) {}

// LaunchSite.cs:160 (the second half of Setup after the PQSCity / PQSCity2 cast)
bool LaunchSite::SetupCommon(const std::vector<space_center::PQSInfo>& PQSs) {
    for (size_t i = 0; i < PQSs.size(); i++) {
        if (PQSs[i].gameObjectName == pqsName) {
            launchsitePQS = PQSs[i].pqs;
            launchSiteTransformFound = false;
            if (SceneGraph::FindInPQS) {
                auto found = SceneGraph::FindInPQS(pqsName, launchSiteTransformURL);
                if (!found) found = SceneGraph::FindInPQS(pqsName, launchSiteTransformURL + "(Clone)");
                if (found) {
                    launchSiteTransformFound = true;
                    launchSiteTransformPosition = *found;
                }
            }
            break;
        }
    }
    if (launchsitePQS == nullptr) {
        log::Error("[LaunchSite]: unable to find LaunchSite PQS : " + pqsName);
        return false;
    }
    if (!launchSiteTransformFound) {
        log::Error("[LaunchSite]: unable to find LaunchSite Transform :" + launchSiteTransformURL);
        return false;
    }
    for (size_t j = 0; j < spawnPoints.size(); j++) spawnPoints[j].Setup(this);
    isSetup_ = true;
    return true;
}

// LaunchSite.cs:160 (Object is a PQSCity)
bool LaunchSite::Setup(PQSCity* city, const std::vector<space_center::PQSInfo>& PQSs) {
    if (city == nullptr) {
        log::Info("[LaunchSite]: pqsCity cannot be null");
        return false;
    }
    pqsCity = city;
    return SetupCommon(PQSs);
}

// LaunchSite.cs:160 (Object is a PQSCity2: `as PQSCity` yields null, which also clears this.pqsCity)
bool LaunchSite::Setup(PQSCity2* city, const std::vector<space_center::PQSInfo>& PQSs) {
    if (city == nullptr) {
        log::Info("[LaunchSite]: pqsCity cannot be null");
        return false;
    }
    pqsCity = nullptr;
    pqsCity2 = city;
    return SetupCommon(PQSs);
}

// LaunchSite.cs:212
LaunchSite::SpawnPoint* LaunchSite::GetSpawnPoint(const std::string& spawnPointName) {
    for (size_t num = 0; num < spawnPoints.size(); num++)
        if (spawnPoints[num].name == spawnPointName) return &spawnPoints[num];
    log::Info("[LaunchSite]: Cannot find spawnpoint named '" + spawnPointName + "'");
    return nullptr;
}

// LaunchSite.cs:232
void LaunchSite::SetSpawnPointsLatLonAlt() {
    for (size_t i = 0; i < spawnPoints.size(); i++) spawnPoints[i].SetSpawnPointLatLonAlt();
}

// LaunchSite.cs:240
std::string LaunchSite::GetName() { return name; }

// LaunchSite.cs:245
std::optional<Vector3d> LaunchSite::GetWorldPos() {
    if (!launchSiteTransformFound && SceneGraph::FindInPQS && Body() != nullptr && Body()->pqsController != nullptr) {
        auto found = SceneGraph::FindInPQS(Body()->pqsController->name, launchSiteTransformURL);
        if (!found) found = SceneGraph::FindInPQS(Body()->pqsController->name, launchSiteTransformURL + "(Clone)");
        if (found) {
            launchSiteTransformFound = true;
            launchSiteTransformPosition = *found;
        }
    }
    if (!launchSiteTransformFound) return std::nullopt;
    return launchSiteTransformPosition;
}

// LaunchSite.cs:258
std::string LaunchSite::NodeCaptionHeader() { return Localizer::Format(launchSiteName); }

// LaunchSite.cs:263
double LaunchSite::GreatCircleDistance(const Vector3d& fromSrfNrm) {
    if (!greatCircleVarsSet_) {
        setupGreatCircleVariables();
        greatCircleVarsSet_ = true;
    }
    if (body_ == nullptr) return -DBL_MAX;   // double.MinValue
    return body_->Radius * dotnet::Math::Acos(Vector3d::Dot(srfNVector_, fromSrfNrm));
}

// LaunchSite.cs:277
void LaunchSite::setupGreatCircleVariables() {
    if (pqsCity2 != nullptr) {
        body_ = pqsCity2->body;
        srfNVector_ = body_->GetRelSurfaceNVector(pqsCity2->lat, pqsCity2->lon);
    } else if (pqsCity != nullptr) {
        body_ = pqsCity->body;
        double lat = 0.0, lon = 0.0, alt = 0.0;
        pqsCity->LatLonAlt(lat, lon, alt);
        srfNVector_ = body_->GetRelSurfaceNVector(lat, lon);
    }
}

// KSCSiteNode.cs:14
std::string KSCSiteNode::GetDisplayName() { return Localizer::Format("#autoLOC_8003001"); }

// KSCSiteNode.cs:19
std::optional<Vector3d> KSCSiteNode::GetWorldPos() {
    PSystemSetup::SpaceCenterFacility* runwayFacility = PSystemSetup::Instance->GetSpaceCenterFacility("Runway");
    if (runwayFacility == nullptr) throw NullReferenceException("runwayFacility");
    if (!runwayFacility->facilityTransformFound) return std::nullopt;
    return runwayFacility->facilityTransformPosition;
}

}  // namespace ksp

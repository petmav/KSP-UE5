// Enums of the save model (WP-04) with their .NET name tables (ksp::dotnet::EnumTraits, declaration order and
// [Description] tags as in the decompiled C#), so Enum.ToString / Enum.Parse behave like in KSP.
// Nested C# enums are declared at namespace scope as Outer_Inner (the convention of ksp/events/game_event_types.hpp)
// and aliased inside their classes.
#pragma once

#include <cstdint>

#include "ksp/dotnet/enum.hpp"
#include "ksp/physics/vessel_ranges.hpp"  // VesselSituations (Vessel.Situations)

namespace ksp {

// GameScenes.cs
enum class GameScenes : int {
    LOADING = 0,
    LOADINGBUFFER = 1,
    MAINMENU = 2,
    SETTINGS = 3,
    CREDITS = 4,
    SPACECENTER = 5,
    EDITOR = 6,
    FLIGHT = 7,
    TRACKSTATION = 8,
    PSYSTEM = 9,
    MISSIONBUILDER = 21,
};

// EditorFacility.cs
enum class EditorFacility : int { None, VAB, SPH };

// SpaceCenterFacility.cs:2 (WP-36 / WP-40 declared it twice; WP-42 needs both headers in one translation unit)
enum class SpaceCenterFacility : int {
    Administration, AstronautComplex, LaunchPad, MissionControl, ResearchAndDevelopment, Runway, TrackingStation, SpaceplaneHangar,
    VehicleAssemblyBuilding
};

// Game.cs:12 Game.Modes / Game.cs:28 Game.GameStatus
enum class Game_Modes : int { SANDBOX, CAREER, SCENARIO, SCENARIO_NON_RESUMABLE, SCIENCE_SANDBOX, MISSION, MISSION_BUILDER };
enum class Game_GameStatus : int { UNSTARTED, ONGOING, FAILED_OR_ABORTED, COMPLETED };

// ProtoCrewMember.cs:14-50
enum class ProtoCrewMember_RosterStatus : int { Available, Assigned, Dead, Missing };
enum class ProtoCrewMember_KerbalType : int { Crew, Applicant, Unowned, Tourist };
enum class ProtoCrewMember_Gender : int { Male, Female };
enum class ProtoCrewMember_KerbalSuit : int { Default, Vintage, Future, Slim };

// VesselType.cs
enum class VesselType : int {
    Debris, SpaceObject, Unknown, Probe, Relay, Rover, Lander, Ship, Plane, Station, Base, EVA, Flag,
    DeployedScienceController, DeployedSciencePart, DroppedPart, DeployedGroundPart
};

// AltimeterDisplayState.cs
enum class AltimeterDisplayState : int { DEFAULT, ASL, AGL };

// KSPActionGroup.cs ([Flags])
enum class KSPActionGroup : int {
    REPLACEWITHDEFAULT = -1, None = 0, Stage = 1, Gear = 2, Light = 4, RCS = 8, SAS = 0x10, Brakes = 0x20, Abort = 0x40,
    Custom01 = 0x80, Custom02 = 0x100, Custom03 = 0x200, Custom04 = 0x400, Custom05 = 0x800, Custom06 = 0x1000,
    Custom07 = 0x2000, Custom08 = 0x4000, Custom09 = 0x8000, Custom10 = 0x10000
};

// KSPAxisGroup.cs ([Flags])
enum class KSPAxisGroup : int {
    REPLACEWITHDEFAULT = -1, None = 0, Pitch = 1, Yaw = 2, Roll = 4, TranslateX = 8, TranslateY = 0x10, TranslateZ = 0x20,
    MainThrottle = 0x40, WheelSteer = 0x80, WheelThrottle = 0x100, Custom01 = 0x200, Custom02 = 0x400, Custom03 = 0x800,
    Custom04 = 0x1000
};

// SymmetryMethod.cs
enum class SymmetryMethod : int { Radial, Mirror };

// Part.cs:72 Part.AutoStrutMode
enum class Part_AutoStrutMode : int { Off, Root, Heaviest, Grandparent, ForceRoot, ForceHeaviest, ForceGrandparent };

// CommNet/CommNetUI.cs:12 CommNetUI.DisplayMode
enum class CommNetUI_DisplayMode : int { None, FirstHop, Path, VesselLinks, Network };

// FlightLog.cs:6 FlightLog.EntryType
enum class FlightLog_EntryType : int {
    Land, Flight, Flyby, Orbit, Suborbit, Escape, Launch, ExitVessel, BoardVessel, PlantFlag, Recover, Die, Spawn,
    Training1, Training2, Training3, Training4, Training5
};

// ProtoTargetInfo.cs:6 / ProtoWaypointInfo.cs:6
enum class ProtoTargetInfo_Type : int { Vessel, PartModule, Part, CelestialBody, Generic, Null };
enum class ProtoWaypointInfo_Type : int { Custom, Survey, Null };

// GameParameters.cs:572 / 586
enum class GameParameters_Preset : int { Easy, Normal, Moderate, Hard, Custom };
enum class GameParameters_GameMode : int { NONE = 0, SANDBOX = 1, SCIENCE = 2, CAREER = 4, MISSION = 8, NOTMISSION = 7, ANY = 15 };

// SaveMode.cs
enum class SaveMode : int { OVERWRITE, APPEND, ABORT, BACKUP };

}  // namespace ksp

namespace ksp::dotnet {

#define KSP_SAVE_ENUM_M(E, n) {E::n, #n}
#define KSP_SAVE_ENUM_D(E, n, d) {E::n, #n, d}

template <>
struct EnumTraits<ksp::GameScenes> {
    using E = ksp::GameScenes;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {
        KSP_SAVE_ENUM_M(E, LOADING), KSP_SAVE_ENUM_M(E, LOADINGBUFFER), KSP_SAVE_ENUM_M(E, MAINMENU),
        KSP_SAVE_ENUM_M(E, SETTINGS), KSP_SAVE_ENUM_M(E, CREDITS), KSP_SAVE_ENUM_M(E, SPACECENTER),
        KSP_SAVE_ENUM_M(E, EDITOR), KSP_SAVE_ENUM_M(E, FLIGHT), KSP_SAVE_ENUM_M(E, TRACKSTATION),
        KSP_SAVE_ENUM_M(E, PSYSTEM), KSP_SAVE_ENUM_M(E, MISSIONBUILDER)};
};
template <>
struct EnumTraits<ksp::EditorFacility> {
    using E = ksp::EditorFacility;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {KSP_SAVE_ENUM_D(E, None, "#autoLOC_6003000"),
                                                KSP_SAVE_ENUM_D(E, VAB, "#autoLOC_6002108"),
                                                KSP_SAVE_ENUM_D(E, SPH, "#autoLOC_6002119")};
};
template <>
struct EnumTraits<ksp::Game_Modes> {
    using E = ksp::Game_Modes;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {
        KSP_SAVE_ENUM_D(E, SANDBOX, "#autoLOC_190706"), KSP_SAVE_ENUM_D(E, CAREER, "#autoLOC_190722"),
        KSP_SAVE_ENUM_D(E, SCENARIO, "#autoLOC_6003000"), KSP_SAVE_ENUM_D(E, SCENARIO_NON_RESUMABLE, "#autoLOC_6003000"),
        KSP_SAVE_ENUM_D(E, SCIENCE_SANDBOX, "#autoLOC_190714"), KSP_SAVE_ENUM_M(E, MISSION),
        KSP_SAVE_ENUM_M(E, MISSION_BUILDER)};
};
template <>
struct EnumTraits<ksp::Game_GameStatus> {
    using E = ksp::Game_GameStatus;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {KSP_SAVE_ENUM_M(E, UNSTARTED), KSP_SAVE_ENUM_M(E, ONGOING),
                                                KSP_SAVE_ENUM_M(E, FAILED_OR_ABORTED), KSP_SAVE_ENUM_M(E, COMPLETED)};
};
template <>
struct EnumTraits<ksp::ProtoCrewMember_RosterStatus> {
    using E = ksp::ProtoCrewMember_RosterStatus;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {
        KSP_SAVE_ENUM_D(E, Available, "#autoLOC_901072"), KSP_SAVE_ENUM_D(E, Assigned, "#autoLOC_901073"),
        KSP_SAVE_ENUM_D(E, Dead, "#autoLOC_901074"), KSP_SAVE_ENUM_D(E, Missing, "#autoLOC_901075")};
};
template <>
struct EnumTraits<ksp::ProtoCrewMember_KerbalType> {
    using E = ksp::ProtoCrewMember_KerbalType;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {
        KSP_SAVE_ENUM_D(E, Crew, "#autoLoc_6002189"), KSP_SAVE_ENUM_D(E, Applicant, "#autoLOC_900290"),
        KSP_SAVE_ENUM_D(E, Unowned, "#autoLOC_901070"), KSP_SAVE_ENUM_D(E, Tourist, "#autoLOC_476080")};
};
template <>
struct EnumTraits<ksp::ProtoCrewMember_Gender> {
    using E = ksp::ProtoCrewMember_Gender;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {KSP_SAVE_ENUM_M(E, Male), KSP_SAVE_ENUM_M(E, Female)};
};
template <>
struct EnumTraits<ksp::ProtoCrewMember_KerbalSuit> {
    using E = ksp::ProtoCrewMember_KerbalSuit;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {KSP_SAVE_ENUM_M(E, Default), KSP_SAVE_ENUM_M(E, Vintage),
                                                KSP_SAVE_ENUM_M(E, Future), KSP_SAVE_ENUM_M(E, Slim)};
};
template <>
struct EnumTraits<ksp::VesselType> {
    using E = ksp::VesselType;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {
        KSP_SAVE_ENUM_D(E, Debris, "#autoLOC_900676"), KSP_SAVE_ENUM_D(E, SpaceObject, "#autoLoc_6002177"),
        KSP_SAVE_ENUM_D(E, Unknown, "#autoLOC_6004021"), KSP_SAVE_ENUM_D(E, Probe, "#autoLOC_900681"),
        KSP_SAVE_ENUM_D(E, Relay, "#autoLOC_900687"), KSP_SAVE_ENUM_D(E, Rover, "#autoLOC_900683"),
        KSP_SAVE_ENUM_D(E, Lander, "#autoLOC_900686"), KSP_SAVE_ENUM_D(E, Ship, "#autoLOC_900684"),
        KSP_SAVE_ENUM_D(E, Plane, "#autoLOC_900685"), KSP_SAVE_ENUM_D(E, Station, "#autoLOC_900679"),
        KSP_SAVE_ENUM_D(E, Base, "#autoLoc_6002178"), KSP_SAVE_ENUM_D(E, EVA, "#autoLOC_6003088"),
        KSP_SAVE_ENUM_D(E, Flag, "#autoLoc_6002179"), KSP_SAVE_ENUM_D(E, DeployedScienceController, "#autoLOC_8002227"),
        KSP_SAVE_ENUM_D(E, DeployedSciencePart, "#autoLOC_8002223"), KSP_SAVE_ENUM_D(E, DroppedPart, "#autoLOC_6006094"),
        KSP_SAVE_ENUM_D(E, DeployedGroundPart, "#autoLOC_6009000")};
};
template <>
struct EnumTraits<ksp::VesselSituations> {
    using E = ksp::VesselSituations;
    static constexpr bool flags = false;  // Vessel.cs:25 has no [Flags] attribute
    static constexpr EnumMember<E> members[] = {
        KSP_SAVE_ENUM_D(E, LANDED, "#autoLoc_6002161"), KSP_SAVE_ENUM_D(E, SPLASHED, "#autoLoc_6002162"),
        KSP_SAVE_ENUM_D(E, PRELAUNCH, "#autoLoc_6002163"), KSP_SAVE_ENUM_D(E, FLYING, "#autoLoc_6002164"),
        KSP_SAVE_ENUM_D(E, SUB_ORBITAL, "#autoLoc_6002165"), KSP_SAVE_ENUM_D(E, ORBITING, "#autoLoc_6002166"),
        KSP_SAVE_ENUM_D(E, ESCAPING, "#autoLoc_6002167"), KSP_SAVE_ENUM_D(E, DOCKED, "#autoLoc_6002168")};
};
template <>
struct EnumTraits<ksp::CommNetUI_DisplayMode> {
    using E = ksp::CommNetUI_DisplayMode;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {
        KSP_SAVE_ENUM_D(E, None, "#autoLOC_6003083"), KSP_SAVE_ENUM_D(E, FirstHop, "#autoLOC_6003084"),
        KSP_SAVE_ENUM_D(E, Path, "#autoLOC_6003085"), KSP_SAVE_ENUM_D(E, VesselLinks, "#autoLOC_6003086"),
        KSP_SAVE_ENUM_D(E, Network, "#autoLOC_6003087")};
};
template <>
struct EnumTraits<ksp::AltimeterDisplayState> {
    using E = ksp::AltimeterDisplayState;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {KSP_SAVE_ENUM_M(E, DEFAULT), KSP_SAVE_ENUM_M(E, ASL),
                                                KSP_SAVE_ENUM_M(E, AGL)};
};
template <>
struct EnumTraits<ksp::KSPActionGroup> {
    using E = ksp::KSPActionGroup;
    static constexpr bool flags = true;
    static constexpr EnumMember<E> members[] = {
        KSP_SAVE_ENUM_M(E, REPLACEWITHDEFAULT), KSP_SAVE_ENUM_D(E, None, "#autoLOC_6003000"),
        KSP_SAVE_ENUM_D(E, Stage, "#autoLOC_6003001"), KSP_SAVE_ENUM_D(E, Gear, "#autoLOC_6003002"),
        KSP_SAVE_ENUM_D(E, Light, "#autoLOC_6003003"), KSP_SAVE_ENUM_D(E, RCS, "#autoLOC_6003004"),
        KSP_SAVE_ENUM_D(E, SAS, "#autoLOC_6003005"), KSP_SAVE_ENUM_D(E, Brakes, "#autoLOC_6003006"),
        KSP_SAVE_ENUM_D(E, Abort, "#autoLOC_6003007"), KSP_SAVE_ENUM_D(E, Custom01, "#autoLOC_6003008"),
        KSP_SAVE_ENUM_D(E, Custom02, "#autoLOC_6003009"), KSP_SAVE_ENUM_D(E, Custom03, "#autoLOC_6003010"),
        KSP_SAVE_ENUM_D(E, Custom04, "#autoLOC_6003011"), KSP_SAVE_ENUM_D(E, Custom05, "#autoLOC_6003012"),
        KSP_SAVE_ENUM_D(E, Custom06, "#autoLOC_6003013"), KSP_SAVE_ENUM_D(E, Custom07, "#autoLOC_6003014"),
        KSP_SAVE_ENUM_D(E, Custom08, "#autoLOC_6003015"), KSP_SAVE_ENUM_D(E, Custom09, "#autoLOC_6003016"),
        KSP_SAVE_ENUM_D(E, Custom10, "#autoLOC_6003017")};
};
template <>
struct EnumTraits<ksp::KSPAxisGroup> {
    using E = ksp::KSPAxisGroup;
    static constexpr bool flags = true;
    static constexpr EnumMember<E> members[] = {
        KSP_SAVE_ENUM_M(E, REPLACEWITHDEFAULT), KSP_SAVE_ENUM_D(E, None, "#autoLOC_6003000"),
        KSP_SAVE_ENUM_D(E, Pitch, "#autoLOC_6013004"), KSP_SAVE_ENUM_D(E, Yaw, "#autoLOC_6013005"),
        KSP_SAVE_ENUM_D(E, Roll, "#autoLOC_6013006"), KSP_SAVE_ENUM_D(E, TranslateX, "#autoLOC_6013007"),
        KSP_SAVE_ENUM_D(E, TranslateY, "#autoLOC_6013008"), KSP_SAVE_ENUM_D(E, TranslateZ, "#autoLOC_6013009"),
        KSP_SAVE_ENUM_D(E, MainThrottle, "#autoLOC_6013010"), KSP_SAVE_ENUM_D(E, WheelSteer, "#autoLOC_6013011"),
        KSP_SAVE_ENUM_D(E, WheelThrottle, "#autoLOC_6013012"), KSP_SAVE_ENUM_D(E, Custom01, "#autoLOC_6013013"),
        KSP_SAVE_ENUM_D(E, Custom02, "#autoLOC_6013014"), KSP_SAVE_ENUM_D(E, Custom03, "#autoLOC_6013015"),
        KSP_SAVE_ENUM_D(E, Custom04, "#autoLOC_6013016")};
};
template <>
struct EnumTraits<ksp::SymmetryMethod> {
    using E = ksp::SymmetryMethod;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {KSP_SAVE_ENUM_M(E, Radial), KSP_SAVE_ENUM_M(E, Mirror)};
};
template <>
struct EnumTraits<ksp::Part_AutoStrutMode> {
    using E = ksp::Part_AutoStrutMode;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {
        KSP_SAVE_ENUM_M(E, Off), KSP_SAVE_ENUM_M(E, Root), KSP_SAVE_ENUM_M(E, Heaviest), KSP_SAVE_ENUM_M(E, Grandparent),
        KSP_SAVE_ENUM_M(E, ForceRoot), KSP_SAVE_ENUM_M(E, ForceHeaviest), KSP_SAVE_ENUM_M(E, ForceGrandparent)};
};
template <>
struct EnumTraits<ksp::FlightLog_EntryType> {
    using E = ksp::FlightLog_EntryType;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {
        KSP_SAVE_ENUM_M(E, Land), KSP_SAVE_ENUM_M(E, Flight), KSP_SAVE_ENUM_M(E, Flyby), KSP_SAVE_ENUM_M(E, Orbit),
        KSP_SAVE_ENUM_M(E, Suborbit), KSP_SAVE_ENUM_M(E, Escape), KSP_SAVE_ENUM_M(E, Launch),
        KSP_SAVE_ENUM_M(E, ExitVessel), KSP_SAVE_ENUM_M(E, BoardVessel), KSP_SAVE_ENUM_M(E, PlantFlag),
        KSP_SAVE_ENUM_M(E, Recover), KSP_SAVE_ENUM_M(E, Die), KSP_SAVE_ENUM_M(E, Spawn), KSP_SAVE_ENUM_M(E, Training1),
        KSP_SAVE_ENUM_M(E, Training2), KSP_SAVE_ENUM_M(E, Training3), KSP_SAVE_ENUM_M(E, Training4),
        KSP_SAVE_ENUM_M(E, Training5)};
};
template <>
struct EnumTraits<ksp::ProtoTargetInfo_Type> {
    using E = ksp::ProtoTargetInfo_Type;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {KSP_SAVE_ENUM_M(E, Vessel), KSP_SAVE_ENUM_M(E, PartModule),
                                                KSP_SAVE_ENUM_M(E, Part), KSP_SAVE_ENUM_M(E, CelestialBody),
                                                KSP_SAVE_ENUM_M(E, Generic), KSP_SAVE_ENUM_M(E, Null)};
};
template <>
struct EnumTraits<ksp::ProtoWaypointInfo_Type> {
    using E = ksp::ProtoWaypointInfo_Type;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {KSP_SAVE_ENUM_M(E, Custom), KSP_SAVE_ENUM_M(E, Survey),
                                                KSP_SAVE_ENUM_M(E, Null)};
};
template <>
struct EnumTraits<ksp::GameParameters_Preset> {
    using E = ksp::GameParameters_Preset;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {
        KSP_SAVE_ENUM_D(E, Easy, "#autoLOC_7000039"), KSP_SAVE_ENUM_D(E, Normal, "#autoLOC_7000040"),
        KSP_SAVE_ENUM_D(E, Moderate, "#autoLOC_7000041"), KSP_SAVE_ENUM_D(E, Hard, "#autoLOC_7000042"),
        KSP_SAVE_ENUM_D(E, Custom, "#autoLOC_7000043")};
};
template <>
struct EnumTraits<ksp::GameParameters_GameMode> {
    using E = ksp::GameParameters_GameMode;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {KSP_SAVE_ENUM_M(E, NONE),    KSP_SAVE_ENUM_M(E, SANDBOX),
                                                KSP_SAVE_ENUM_M(E, SCIENCE), KSP_SAVE_ENUM_M(E, CAREER),
                                                KSP_SAVE_ENUM_M(E, MISSION), KSP_SAVE_ENUM_M(E, NOTMISSION),
                                                KSP_SAVE_ENUM_M(E, ANY)};
};
template <>
struct EnumTraits<ksp::SaveMode> {
    using E = ksp::SaveMode;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {KSP_SAVE_ENUM_M(E, OVERWRITE), KSP_SAVE_ENUM_M(E, APPEND),
                                                KSP_SAVE_ENUM_M(E, ABORT), KSP_SAVE_ENUM_M(E, BACKUP)};
};

#undef KSP_SAVE_ENUM_M
#undef KSP_SAVE_ENUM_D

}  // namespace ksp::dotnet

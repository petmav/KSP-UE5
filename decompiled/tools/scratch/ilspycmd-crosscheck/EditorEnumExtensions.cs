using System;
using KSP.Localization;
using UnityEngine;

public static class EditorEnumExtensions
{
	public static SpaceCenterFacility ToFacility(this EditorFacility editor)
	{
		switch (editor)
		{
		default:
			return SpaceCenterFacility.SpaceplaneHangar;
		case EditorFacility.VAB:
			return SpaceCenterFacility.VehicleAssemblyBuilding;
		}
	}

	public static EditorFacility ToEditor(this SpaceCenterFacility facility)
	{
		switch (facility)
		{
		default:
			return EditorFacility.None;
		case SpaceCenterFacility.Runway:
		case SpaceCenterFacility.SpaceplaneHangar:
			return EditorFacility.SPH;
		case SpaceCenterFacility.LaunchPad:
		case SpaceCenterFacility.VehicleAssemblyBuilding:
			return EditorFacility.VAB;
		}
	}

	public static SpaceCenterFacility GetEditorFacility(this SpaceCenterFacility launchSite)
	{
		switch (launchSite)
		{
		case SpaceCenterFacility.Runway:
		case SpaceCenterFacility.SpaceplaneHangar:
			return SpaceCenterFacility.SpaceplaneHangar;
		default:
			return SpaceCenterFacility.VehicleAssemblyBuilding;
		}
	}

	public static SpaceCenterFacility GetFacility(this PSystemSetup.SpaceCenterFacility pSystemFacility)
	{
		switch (pSystemFacility.name)
		{
		case "LaunchPad":
			return SpaceCenterFacility.LaunchPad;
		case "ResearchAndDevelopment":
		case "RnD":
			return SpaceCenterFacility.ResearchAndDevelopment;
		case "TrackingStation":
			return SpaceCenterFacility.TrackingStation;
		case "Runway":
			return SpaceCenterFacility.Runway;
		case "VAB":
		case "VehicleAssemblyBuilding":
			return SpaceCenterFacility.VehicleAssemblyBuilding;
		case "Administration":
			return SpaceCenterFacility.Administration;
		case "FlagPole":
			Debug.LogError("FlagPole is not an actual Space Center Facility!");
			return SpaceCenterFacility.AstronautComplex;
		case "SPH":
		case "SpaceplaneHangar":
			return SpaceCenterFacility.SpaceplaneHangar;
		default:
			throw new ArgumentNullException("pSystemFacility", pSystemFacility.name + " is not a valid Space Center Facility name!");
		case "MissionControl":
			return SpaceCenterFacility.MissionControl;
		}
	}

	public static string GetSymmetryMethodName(SymmetryMethod method)
	{
		switch (method)
		{
		default:
			return "Unlocalised symmetry method!";
		case SymmetryMethod.Mirror:
			return Localizer.Format("#autoLOC_6004040");
		case SymmetryMethod.Radial:
			return Localizer.Format("#autoLOC_6004039");
		}
	}
}

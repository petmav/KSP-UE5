using System.Collections.Generic;
using CommNet;
using KSP.Localization;

namespace PreFlightTests
{
	public class AntennaPresent : DesignConcernBase
	{
		private ShipConstruct ship;

		private bool foundShortRanged;

		private bool foundLongRanged;

		private bool foundCrewedPart;

		private static string cacheAutoLOC_251018;

		private static string cacheAutoLOC_251023;

		private static string cacheAutoLOC_251025;

		private static string cacheAutoLOC_251028;

		private static string cacheAutoLOC_251036;

		private static string cacheAutoLOC_251041;

		private static string cacheAutoLOC_251043;

		private static string cacheAutoLOC_251046;

		public override bool TestCondition()
		{
			if (!CommNetScenario.CommNetEnabled)
			{
				return true;
			}
			ship = EditorLogic.fetch.ship;
			VesselCrewManifest shipManifest = ShipConstruction.ShipManifest;
			foundShortRanged = false;
			foundLongRanged = false;
			foundCrewedPart = false;
			int i = 0;
			for (int count = ship.Count; i < count; i++)
			{
				Part part = ship[i];
				if (!foundCrewedPart && PartIsCrewControlled(part, shipManifest))
				{
					foundCrewedPart = true;
				}
				if (!foundShortRanged || !foundLongRanged)
				{
					List<ICommAntenna> list = part.FindModulesImplementing<ICommAntenna>();
					int num = list.Count - 1;
					while (num >= 0 && (!foundShortRanged || !foundLongRanged))
					{
						if (list[num].CommType == AntennaType.INTERNAL)
						{
							foundShortRanged = true;
						}
						else
						{
							foundLongRanged = true;
						}
						num--;
					}
				}
				if (foundShortRanged && foundLongRanged && foundCrewedPart)
				{
					break;
				}
			}
			if (foundCrewedPart && !foundLongRanged)
			{
				return false;
			}
			if (!foundCrewedPart && (!foundShortRanged || !foundLongRanged))
			{
				return false;
			}
			return true;
		}

		private bool PartIsCrewControlled(Part p, VesselCrewManifest manifest)
		{
			if (manifest == null)
			{
				return false;
			}
			PartCrewManifest partCrewManifest = manifest.GetPartCrewManifest(p.craftID);
			if (partCrewManifest != null && partCrewManifest.PartInfo.partPrefab.CrewCapacity > 0 && p.CrewCapacity > 0 && !partCrewManifest.AllSeatsEmpty())
			{
				if (partCrewManifest.PartInfo.partPrefab.isControlSource <= Vessel.ControlLevel.NONE)
				{
					return false;
				}
				ModuleCommand moduleCommand = partCrewManifest.PartInfo.partPrefab.FindModuleImplementing<ModuleCommand>();
				if (moduleCommand != null)
				{
					if (moduleCommand.minimumCrew <= partCrewManifest.CountCrewNotType(ProtoCrewMember.KerbalType.Tourist))
					{
						return true;
					}
					return false;
				}
				return true;
			}
			return false;
		}

		public override string GetConcernTitle()
		{
			if (foundCrewedPart)
			{
				if (!foundLongRanged)
				{
					return cacheAutoLOC_251018;
				}
			}
			else
			{
				if (!foundShortRanged)
				{
					return cacheAutoLOC_251023;
				}
				if (!foundLongRanged)
				{
					return cacheAutoLOC_251025;
				}
			}
			return cacheAutoLOC_251028;
		}

		public override string GetConcernDescription()
		{
			if (foundCrewedPart)
			{
				if (!foundLongRanged)
				{
					return cacheAutoLOC_251036;
				}
			}
			else
			{
				if (!foundShortRanged)
				{
					return cacheAutoLOC_251041;
				}
				if (!foundLongRanged)
				{
					return cacheAutoLOC_251043;
				}
			}
			return cacheAutoLOC_251046;
		}

		public override DesignConcernSeverity GetSeverity()
		{
			if (foundCrewedPart)
			{
				return DesignConcernSeverity.NOTICE;
			}
			if (!foundShortRanged)
			{
				return DesignConcernSeverity.CRITICAL;
			}
			if (!foundLongRanged)
			{
				return DesignConcernSeverity.WARNING;
			}
			return DesignConcernSeverity.NOTICE;
		}

		internal static void CacheLocalStrings()
		{
			cacheAutoLOC_251018 = Localizer.Format("#autoLOC_251018");
			cacheAutoLOC_251023 = Localizer.Format("#autoLOC_251023");
			cacheAutoLOC_251025 = Localizer.Format("#autoLOC_251025");
			cacheAutoLOC_251028 = Localizer.Format("#autoLOC_251028");
			cacheAutoLOC_251036 = Localizer.Format("#autoLOC_251036");
			cacheAutoLOC_251041 = Localizer.Format("#autoLOC_251041");
			cacheAutoLOC_251043 = Localizer.Format("#autoLOC_251043");
			cacheAutoLOC_251046 = Localizer.Format("#autoLOC_251046");
		}
	}
}

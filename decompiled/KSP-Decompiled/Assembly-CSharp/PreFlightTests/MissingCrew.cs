using System.Collections.Generic;
using KSP.Localization;

namespace PreFlightTests
{
	public class MissingCrew : DesignConcernBase
	{
		private ShipConstruct ship;

		private VesselCrewManifest manifest;

		private List<Part> failedParts = new List<Part>();

		private static string cacheAutoLOC_252504;

		private static string cacheAutoLOC_252505;

		private static string cacheAutoLOC_252511;

		private static string cacheAutoLOC_252512;

		public MissingCrew()
		{
			manifest = ShipConstruction.ShipManifest;
		}

		public override bool TestCondition()
		{
			ship = EditorLogic.fetch.ship;
			failedParts.Clear();
			manifest = ShipConstruction.ShipManifest;
			if (manifest == null)
			{
				return false;
			}
			int count = ship.Count;
			for (int i = 0; i < count; i++)
			{
				Part part = ship[i];
				PartCrewManifest partCrewManifest = manifest.GetPartCrewManifest(part.craftID);
				if (partCrewManifest != null && partCrewManifest.PartInfo.partPrefab.CrewCapacity > 0 && part.CrewCapacity > 0 && partCrewManifest.AllSeatsEmpty())
				{
					failedParts.Add(part);
				}
			}
			return failedParts.Count == 0;
		}

		public override string GetConcernTitle()
		{
			if (failedParts.Count == 1)
			{
				return cacheAutoLOC_252504;
			}
			return cacheAutoLOC_252505;
		}

		public override string GetConcernDescription()
		{
			if (failedParts.Count == 1)
			{
				return cacheAutoLOC_252511;
			}
			return cacheAutoLOC_252512;
		}

		public override DesignConcernSeverity GetSeverity()
		{
			return DesignConcernSeverity.NOTICE;
		}

		public override List<Part> GetAffectedParts()
		{
			return failedParts;
		}

		internal static void CacheLocalStrings()
		{
			cacheAutoLOC_252504 = Localizer.Format("#autoLOC_252504");
			cacheAutoLOC_252505 = Localizer.Format("#autoLOC_252505");
			cacheAutoLOC_252511 = Localizer.Format("#autoLOC_252511");
			cacheAutoLOC_252512 = Localizer.Format("#autoLOC_252512");
		}
	}
}

using System.Collections.Generic;
using KSP.Localization;
using UnityEngine;

namespace PreFlightTests
{
	public class NonRootCmdMissaligned : DesignConcernBase
	{
		private ShipConstruct ship;

		private List<Part> failedParts = new List<Part>();

		private static string cacheAutoLOC_252566;

		private static string cacheAutoLOC_252567;

		private static string cacheAutoLOC_252573;

		private static string cacheAutoLOC_252574;

		public override bool TestCondition()
		{
			ship = EditorLogic.fetch.ship;
			failedParts.Clear();
			int count = ship.Count;
			for (int i = 0; i < count; i++)
			{
				Part part = ship[i];
				if (part != EditorLogic.RootPart && part.FindModuleImplementing<ModuleCommand>() != null && (Vector3.Angle(part.transform.rotation * Vector3.up, EditorLogic.RootPart.transform.rotation * Vector3.up) > 5f || Vector3.Angle(part.transform.rotation * Vector3.forward, EditorLogic.RootPart.transform.rotation * Vector3.forward) > 5f))
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
				return cacheAutoLOC_252566;
			}
			return cacheAutoLOC_252567;
		}

		public override string GetConcernDescription()
		{
			if (failedParts.Count == 1)
			{
				return cacheAutoLOC_252573;
			}
			return cacheAutoLOC_252574;
		}

		public override DesignConcernSeverity GetSeverity()
		{
			return DesignConcernSeverity.WARNING;
		}

		public override List<Part> GetAffectedParts()
		{
			return failedParts;
		}

		internal static void CacheLocalStrings()
		{
			cacheAutoLOC_252566 = Localizer.Format("#autoLOC_252566");
			cacheAutoLOC_252567 = Localizer.Format("#autoLOC_252567");
			cacheAutoLOC_252573 = Localizer.Format("#autoLOC_252573");
			cacheAutoLOC_252574 = Localizer.Format("#autoLOC_252574");
		}
	}
}

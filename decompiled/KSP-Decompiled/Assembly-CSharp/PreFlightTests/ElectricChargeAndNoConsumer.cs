using System.Collections.Generic;
using KSP.Localization;

namespace PreFlightTests
{
	public class ElectricChargeAndNoConsumer : DesignConcernBase
	{
		private ShipConstruct ship;

		private List<Part> failedParts = new List<Part>();

		private PartResourceDefinition resourceDefinition;

		private static string cacheAutoLOC_251969;

		private static string cacheAutoLOC_251970;

		private static string cacheAutoLOC_251976;

		private static string cacheAutoLOC_251977;

		public ElectricChargeAndNoConsumer()
		{
			resourceDefinition = PartResourceLibrary.Instance.GetDefinition("ElectricCharge");
		}

		public override bool TestCondition()
		{
			ship = EditorLogic.fetch.ship;
			failedParts.Clear();
			int count = ship.Count;
			for (int i = 0; i < count; i++)
			{
				Part part = ship[i];
				if (part.HasModuleImplementing<ModuleDeployableSolarPanel>() || (part.name != "launchClamp1" && part.isGenerator(out var generator) && RUIutils.Any(generator.resHandler.outputResources, (ModuleResource a) => a.name == "ElectricCharge")))
				{
					failedParts.Add(part);
				}
			}
			if (failedParts.Count > 0)
			{
				for (int j = 0; j < count; j++)
				{
					Part part = ship[j];
					if (!(part.FindModuleImplementing<ModuleCommand>() != null) && RUIutils.Any(part.Modules.GetModules<IResourceConsumer>(), (IResourceConsumer a) => a.GetConsumedResources().Contains(resourceDefinition)))
					{
						failedParts.Clear();
						return true;
					}
				}
			}
			return failedParts.Count == 0;
		}

		public override string GetConcernTitle()
		{
			if (failedParts.Count == 1)
			{
				return cacheAutoLOC_251969;
			}
			return cacheAutoLOC_251970;
		}

		public override string GetConcernDescription()
		{
			if (failedParts.Count == 1)
			{
				return cacheAutoLOC_251976;
			}
			return cacheAutoLOC_251977;
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
			cacheAutoLOC_251969 = Localizer.Format("#autoLOC_251969");
			cacheAutoLOC_251970 = Localizer.Format("#autoLOC_251970");
			cacheAutoLOC_251976 = Localizer.Format("#autoLOC_251976");
			cacheAutoLOC_251977 = Localizer.Format("#autoLOC_251977");
		}
	}
}

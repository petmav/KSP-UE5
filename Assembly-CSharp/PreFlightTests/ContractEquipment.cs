using System.Collections.Generic;
using KSP.Localization;

namespace PreFlightTests
{
	public class ContractEquipment : DesignConcernBase
	{
		private ShipConstruct ship;

		private List<Part> failedParts = new List<Part>();

		private static string cacheAutoLOC_251140;

		private static string cacheAutoLOC_251145;

		public ContractEquipment(ShipConstruct ship)
		{
			this.ship = ship;
		}

		public override bool TestCondition()
		{
			if (ship.parts.Count > 0 && HighLogic.CurrentGame.Mode == Game.Modes.CAREER)
			{
				failedParts.Clear();
			}
			return failedParts.Count == 0;
		}

		public override string GetConcernTitle()
		{
			return cacheAutoLOC_251140;
		}

		public override string GetConcernDescription()
		{
			return cacheAutoLOC_251145;
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
			cacheAutoLOC_251140 = Localizer.Format("#autoLOC_251140");
			cacheAutoLOC_251145 = Localizer.Format("#autoLOC_251145");
		}
	}
}

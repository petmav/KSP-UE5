using System;
using KSP.Localization;

namespace Contracts.Parameters
{
	[Serializable]
	public class EnterSOI : ContractParameter
	{
		protected CelestialBody targetBody;

		public CelestialBody TargetBody => targetBody;

		public EnterSOI()
		{
		}

		public EnterSOI(CelestialBody targetBody)
		{
			this.targetBody = targetBody;
		}

		protected override string GetHashString()
		{
			return targetBody.name;
		}

		protected override string GetTitle()
		{
			return Localizer.Format("#autoLOC_269618", targetBody.displayName);
		}

		protected override void OnLoad(ConfigNode node)
		{
			if (node.HasValue("body"))
			{
				targetBody = FlightGlobals.fetch.bodies[int.Parse(node.GetValue("body"))];
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("body", targetBody.flightGlobalsIndex);
		}

		protected override void OnRegister()
		{
			GameEvents.VesselSituation.onFlyBy.Add(OnEnterSOI);
		}

		protected override void OnUnregister()
		{
			GameEvents.VesselSituation.onFlyBy.Remove(OnEnterSOI);
		}

		private void OnEnterSOI(Vessel vessel, CelestialBody body)
		{
			if (body == targetBody)
			{
				SetComplete();
			}
		}
	}
}

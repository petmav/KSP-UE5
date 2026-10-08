using System;
using KSP.Localization;

namespace Contracts.Parameters
{
	[Serializable]
	public class EnterOrbit : ContractParameter
	{
		protected CelestialBody targetBody;

		public CelestialBody TargetBody => targetBody;

		public EnterOrbit()
		{
		}

		public EnterOrbit(CelestialBody targetBody)
		{
			this.targetBody = targetBody;
		}

		protected override string GetHashString()
		{
			return targetBody.name;
		}

		protected override string GetTitle()
		{
			return Localizer.Format("#autoLOC_269557", targetBody.displayName);
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
			GameEvents.VesselSituation.onOrbit.Add(OnEnterOrbit);
		}

		protected override void OnUnregister()
		{
			GameEvents.VesselSituation.onOrbit.Remove(OnEnterOrbit);
		}

		private void OnEnterOrbit(Vessel vessel, CelestialBody body)
		{
			if (body == targetBody)
			{
				SetComplete();
			}
		}
	}
}

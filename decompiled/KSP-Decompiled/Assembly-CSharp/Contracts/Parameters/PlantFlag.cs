using System;
using KSP.Localization;

namespace Contracts.Parameters
{
	[Serializable]
	public class PlantFlag : ContractParameter
	{
		protected CelestialBody targetBody;

		public CelestialBody TargetBody => targetBody;

		public PlantFlag()
		{
		}

		public PlantFlag(CelestialBody targetBody)
		{
			this.targetBody = targetBody;
		}

		protected override string GetHashString()
		{
			return targetBody.name;
		}

		protected override string GetTitle()
		{
			return Localizer.Format("#autoLOC_270135", targetBody.displayName);
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
			GameEvents.onFlagPlant.Add(OnPlantFlag);
		}

		protected override void OnUnregister()
		{
			GameEvents.onFlagPlant.Remove(OnPlantFlag);
		}

		private void OnPlantFlag(Vessel vessel)
		{
			if (vessel.orbit.referenceBody == targetBody)
			{
				SetComplete();
			}
		}
	}
}

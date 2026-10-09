using System;
using System.Globalization;
using Contracts;
using FinePrint.Utilities;
using KSP.Localization;

namespace FinePrint.Contracts.Parameters
{
	public class ResourcePossessionParameter : ContractParameter
	{
		private int successCounter;

		private double currentResource;

		public double goalResource;

		public string resourceName;

		public string resourceTitle;

		public string vesselName;

		private SpecificVesselParameter specificVesselSibling;

		private SpecificVesselParameter SpecificVesselSibling => specificVesselSibling ?? (specificVesselSibling = base.Root.GetParameter<SpecificVesselParameter>());

		public ResourcePossessionParameter()
		{
			successCounter = 0;
			currentResource = 0.0;
			goalResource = 1000.0;
			resourceName = "Matter";
			resourceTitle = "matter";
			vesselName = "vessel";
		}

		public ResourcePossessionParameter(string resourceName, string resourceTitle, string vesselName, double goalResource)
		{
			successCounter = 0;
			currentResource = 0.0;
			this.goalResource = goalResource;
			this.resourceName = resourceName;
			this.resourceTitle = resourceTitle;
			this.vesselName = vesselName;
		}

		protected override string GetHashString()
		{
			return SystemUtilities.SuperSeed(base.Root).ToString(CultureInfo.InvariantCulture) + base.ID;
		}

		protected override string GetTitle()
		{
			return Localizer.Format("#autoLOC_284677", Convert.ToDecimal(Math.Round(goalResource)).ToString("#,###"), resourceTitle, vesselName);
		}

		protected override string GetNotes()
		{
			if (!base.Root.IsFinished() && SpecificVesselSibling != null && !(SpecificVesselSibling.targetVessel == null))
			{
				double num = VesselUtilities.VesselResourceAmount(resourceName, SpecificVesselSibling.targetVessel);
				if (num < 0.0)
				{
					return null;
				}
				if (num < 1.0)
				{
					return Localizer.Format("#autoLOC_284691", SpecificVesselSibling.targetVesselName, resourceTitle);
				}
				return Localizer.Format("#autoLOC_284693", SpecificVesselSibling.targetVesselName, Convert.ToDecimal(Math.Round(num)).ToString("#,###"), resourceTitle);
			}
			return null;
		}

		protected override void OnRegister()
		{
			base.DisableOnStateChange = false;
		}

		protected override void OnReset()
		{
			SetIncomplete();
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("goalResource", goalResource);
			node.AddValue("resourceName", resourceName);
			node.AddValue("resourceTitle", resourceTitle);
			node.AddValue("vesselName", vesselName);
		}

		protected override void OnLoad(ConfigNode node)
		{
			SystemUtilities.LoadNode(node, "ResourcePossessionParameter", "goalResource", ref goalResource, 1000.0);
			SystemUtilities.LoadNode(node, "ResourcePossessionParameter", "resourceName", ref resourceName, "Matter");
			SystemUtilities.LoadNode(node, "ResourcePossessionParameter", "resourceTitle", ref resourceTitle, "matter");
			SystemUtilities.LoadNode(node, "ResourcePossessionParameter", "vesselName", ref vesselName, "vessel");
		}

		protected override void OnUpdate()
		{
			if (!SystemUtilities.FlightIsReady(base.Root.ContractState, Contract.State.Active, checkVessel: true))
			{
				return;
			}
			currentResource = VesselUtilities.VesselResourceAmount(resourceName);
			bool flag = Math.Ceiling(currentResource) >= goalResource;
			if (base.State == ParameterState.Incomplete)
			{
				if (flag)
				{
					successCounter++;
				}
				else
				{
					successCounter = 0;
				}
				if (successCounter >= 5)
				{
					SetComplete();
				}
			}
			if (base.State == ParameterState.Complete && !flag)
			{
				SetIncomplete();
			}
		}
	}
}

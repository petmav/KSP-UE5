using System.Collections.Generic;
using System.Globalization;
using Contracts;
using FinePrint.Utilities;
using KSP.Localization;

namespace FinePrint.Contracts.Parameters
{
	public class CometParameter : ContractParameter
	{
		private bool forStation;

		private List<int> cometSeeds;

		private string cometClass;

		private int successCounter;

		private bool validVessel;

		private bool dirtyVessel = true;

		private int activePartCount;

		private bool eventsAdded;

		public CometParameter()
		{
			cometSeeds = new List<int>();
			forStation = false;
			cometClass = "A";
		}

		public CometParameter(string size, bool forStation)
		{
			cometSeeds = new List<int>();
			this.forStation = forStation;
			cometClass = size.ToUpper();
		}

		protected override string GetHashString()
		{
			return SystemUtilities.SuperSeed(base.Root).ToString(CultureInfo.InvariantCulture) + base.ID;
		}

		protected override string GetTitle()
		{
			if (forStation)
			{
				return Localizer.Format("#autoLOC_282737", cometClass);
			}
			return Localizer.Format("#autoLOC_282739", cometClass);
		}

		protected override string GetNotes()
		{
			return Localizer.Format("#autoLOC_282744", base.Root.Agent.Title, cometClass);
		}

		protected override void OnRegister()
		{
			base.DisableOnStateChange = false;
			eventsAdded = false;
			if (base.Root.ContractState == Contract.State.Active)
			{
				GameEvents.onPartCouple.Add(OnDock);
				GameEvents.onVesselWasModified.Add(VesselModified);
				GameEvents.onPartDie.Add(PartModified);
				eventsAdded = true;
			}
		}

		protected override void OnUnregister()
		{
			if (eventsAdded)
			{
				GameEvents.onPartCouple.Remove(OnDock);
				GameEvents.onVesselWasModified.Remove(VesselModified);
				GameEvents.onPartDie.Remove(PartModified);
			}
		}

		protected override void OnReset()
		{
			SetIncomplete();
			dirtyVessel = true;
		}

		private void VesselModified(Vessel v)
		{
			if (v == FlightGlobals.ActiveVessel)
			{
				dirtyVessel = true;
			}
		}

		private void PartModified(Part p)
		{
			if (p.vessel == FlightGlobals.ActiveVessel)
			{
				dirtyVessel = true;
			}
		}

		private void OnDock(GameEvents.FromToAction<Part, Part> action)
		{
			if (!SystemUtilities.FlightIsReady(base.Root.ContractState, Contract.State.Active, checkVessel: true))
			{
				return;
			}
			if (cometSeeds == null)
			{
				cometSeeds = new List<int>();
			}
			List<ModuleComet> list = action.from.vessel.FindPartModulesImplementing<ModuleComet>();
			int count = list.Count;
			while (count-- > 0)
			{
				ModuleComet moduleComet = list[count];
				if (moduleComet.vessel.DiscoveryInfo.Level.ToString() != "Owned")
				{
					if (moduleComet.vessel.DiscoveryInfo.size.Value == cometClass)
					{
						cometSeeds.Add(moduleComet.seed);
						ScreenMessages.PostScreenMessage(Localizer.Format("#autoLOC_282809", base.Root.Agent.Title), 5f, ScreenMessageStyle.UPPER_LEFT);
					}
					else
					{
						ScreenMessages.PostScreenMessage(Localizer.Format("#autoLOC_282812", base.Root.Agent.Title), 5f, ScreenMessageStyle.UPPER_LEFT);
					}
					continue;
				}
				bool flag = false;
				int count2 = cometSeeds.Count;
				while (count2-- > 0)
				{
					if (moduleComet.seed == cometSeeds[count2])
					{
						flag = true;
					}
				}
				if (!flag)
				{
					ScreenMessages.PostScreenMessage(Localizer.Format("#autoLOC_282825", base.Root.Agent.Title), 5f, ScreenMessageStyle.UPPER_LEFT);
				}
				else
				{
					ScreenMessages.PostScreenMessage(Localizer.Format("#autoLOC_282827", base.Root.Agent.Title), 5f, ScreenMessageStyle.UPPER_LEFT);
				}
			}
			if (action.from.vessel == FlightGlobals.ActiveVessel || action.to.vessel == FlightGlobals.ActiveVessel)
			{
				dirtyVessel = true;
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("forStation", forStation);
			node.AddValue("cometClass", cometClass);
			SystemUtilities.SaveNodeList(node, "seedList", cometSeeds);
		}

		protected override void OnLoad(ConfigNode node)
		{
			SystemUtilities.LoadNode(node, "CometParameter", "forStation", ref forStation, defaultValue: false);
			SystemUtilities.LoadNode(node, "CometParameter", "cometClass", ref cometClass, "A");
			SystemUtilities.LoadNodeList(node, "CometParameter", "seedList", ref cometSeeds);
		}

		protected override void OnUpdate()
		{
			if (!SystemUtilities.FlightIsReady(base.Root.ContractState, Contract.State.Active, checkVessel: true))
			{
				return;
			}
			int num = (FlightGlobals.ActiveVessel.loaded ? FlightGlobals.ActiveVessel.Parts.Count : FlightGlobals.ActiveVessel.protoVessel.protoPartSnapshots.Count);
			if (activePartCount != num)
			{
				dirtyVessel = true;
			}
			if (dirtyVessel)
			{
				validVessel = VesselTowingComet(FlightGlobals.ActiveVessel);
			}
			if (base.State == ParameterState.Incomplete)
			{
				if (validVessel)
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
			if (base.State == ParameterState.Complete && !validVessel)
			{
				SetIncomplete();
			}
		}

		private bool VesselTowingComet(Vessel v)
		{
			dirtyVessel = false;
			activePartCount = (v.loaded ? v.Parts.Count : v.protoVessel.protoPartSnapshots.Count);
			if (cometSeeds == null)
			{
				return false;
			}
			List<ModuleComet> list = v.FindPartModulesImplementing<ModuleComet>();
			int count = list.Count;
			while (count-- > 0)
			{
				int count2 = cometSeeds.Count;
				while (count2-- > 0)
				{
					if (list[count].seed == cometSeeds[count2])
					{
						return true;
					}
				}
			}
			return false;
		}
	}
}

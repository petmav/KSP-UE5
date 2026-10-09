using System.Globalization;
using Contracts;
using FinePrint.Utilities;
using KSP.Localization;

namespace FinePrint.Contracts.Parameters
{
	public class KerbalGeeAdventureParameter : ContractParameter
	{
		public string kerbalName;

		private bool eventsAdded;

		public KerbalGeeAdventureParameter()
		{
			kerbalName = "Jebediah Kerman";
		}

		public KerbalGeeAdventureParameter(string kerbalName)
		{
			this.kerbalName = kerbalName;
			allowPartialFailure = true;
		}

		protected override string GetHashString()
		{
			return SystemUtilities.SuperSeed(base.Root).ToString(CultureInfo.InvariantCulture) + base.ID;
		}

		protected override string GetTitle()
		{
			return Localizer.Format("#autoLOC_283371", StringUtilities.ShortKerbalName(kerbalName));
		}

		protected override void OnRegister()
		{
			base.DisableOnStateChange = false;
			if (base.Root.ContractState == Contract.State.Active)
			{
				GameEvents.onCrewKilled.Add(OnCrewKilled);
				GameEvents.onKerbalPassedOutFromGeeForce.Add(OnKerbalPassedOut);
				eventsAdded = true;
			}
		}

		protected override void OnUnregister()
		{
			if (eventsAdded)
			{
				GameEvents.onCrewKilled.Remove(OnCrewKilled);
				GameEvents.onKerbalPassedOutFromGeeForce.Remove(OnKerbalPassedOut);
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("kerbalName", kerbalName);
		}

		protected override void OnLoad(ConfigNode node)
		{
			SystemUtilities.LoadNode(node, "KerbalGeeAdventureParameter", "kerbalName", ref kerbalName, "Jebediah Kerman");
		}

		private void OnCrewKilled(EventReport evt)
		{
			if (evt.sender == kerbalName)
			{
				if (state != ParameterState.Failed)
				{
					allowPartialFailure = false;
					SetFailed();
				}
				else
				{
					base.Root.Fail();
				}
			}
		}

		protected void OnKerbalPassedOut(ProtoCrewMember pcm)
		{
			if (pcm.name == kerbalName && AllChildParametersComplete())
			{
				if (pcm != null && pcm.type == ProtoCrewMember.KerbalType.Tourist)
				{
					pcm.hasToured = true;
				}
				SetComplete();
			}
		}
	}
}

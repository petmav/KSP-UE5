using System.Globalization;
using Contracts;
using FinePrint.Utilities;
using KSP.Localization;
using SentinelMission;

namespace FinePrint.Contracts.Parameters
{
	public class CometDetectionParameter : ContractParameter
	{
		private int cometCountRequired;

		private int cometCountDetected;

		private string sentinelPartName;

		public int RemainingDiscoveries;

		public CometDetectionParameter()
		{
			cometCountDetected = 0;
		}

		public CometDetectionParameter(int cometCountRequired, string sentinelPartName)
		{
			this.cometCountRequired = cometCountRequired;
			cometCountDetected = 0;
			this.sentinelPartName = sentinelPartName;
		}

		protected override string GetHashString()
		{
			return SystemUtilities.SuperSeed(base.Root).ToString(CultureInfo.InvariantCulture) + base.ID;
		}

		protected override string GetTitle()
		{
			return Localizer.Format("#autoLOC_6011147", cometCountRequired.ToString());
		}

		protected override string GetNotes()
		{
			return ((HighLogic.LoadedScene == GameScenes.SPACECENTER) ? "\n" : "") + Localizer.Format("#autoLOC_8003406");
		}

		protected override void OnRegister()
		{
			base.DisableOnStateChange = false;
			GameEvents.onSentinelCometDetected.Add(CometDetected);
		}

		protected override void OnUnregister()
		{
			GameEvents.onSentinelCometDetected.Remove(CometDetected);
		}

		private void CometDetected(Vessel data)
		{
			cometCountDetected++;
			RemainingDiscoveries = cometCountRequired - cometCountDetected;
			if (RemainingDiscoveries > 0)
			{
				string text = Localizer.Format("#autoLOC_8003397", cometCountDetected, cometCountRequired, base.Root.Agent.Title);
				ScreenMessages.PostScreenMessage(text, SentinelUtilities.CalculateReadDuration(text), ScreenMessageStyle.UPPER_CENTER);
			}
			else
			{
				string text2 = Localizer.Format("#autoLOC_8003398", base.Root.Agent.Title);
				ScreenMessages.PostScreenMessage(text2, SentinelUtilities.CalculateReadDuration(text2), ScreenMessageStyle.UPPER_CENTER);
				SetComplete();
				base.Root.Complete();
			}
		}

		protected override void OnReset()
		{
			SetIncomplete();
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("cometCount", cometCountDetected);
			node.AddValue("cometCountRequired", cometCountRequired);
		}

		protected override void OnLoad(ConfigNode node)
		{
			SystemUtilities.LoadNode(node, "CometDetectionParameter", "cometCount", ref cometCountDetected, 1);
			SystemUtilities.LoadNode(node, "CometDetectionParameter", "cometCountRequired", ref cometCountRequired, 1);
		}
	}
}

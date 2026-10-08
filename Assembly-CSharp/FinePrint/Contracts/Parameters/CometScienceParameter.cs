using System;
using Contracts;
using KSP.Localization;

namespace FinePrint.Contracts.Parameters
{
	[Serializable]
	public class CometScienceParameter : ContractParameter
	{
		protected uint cometPersistentId;

		protected string cometName;

		protected string scienceSubjectId;

		protected bool scienceDataCollected;

		public uint CometPersistentId => cometPersistentId;

		public string CometName => cometName;

		public string ScienceSubjectId => scienceSubjectId;

		public bool ScienceDataCollected => scienceDataCollected;

		public CometScienceParameter()
		{
		}

		public CometScienceParameter(uint cometId, string cometName, string scienceSubjectId)
		{
			cometPersistentId = cometId;
			this.cometName = cometName;
			this.scienceSubjectId = scienceSubjectId;
			scienceDataCollected = false;
		}

		protected override string GetHashString()
		{
			return cometName + cometPersistentId;
		}

		protected override string GetTitle()
		{
			return Localizer.Format("#autoLOC_8012045", cometName);
		}

		protected override void OnLoad(ConfigNode node)
		{
			node.TryGetValue("cometName", ref cometName);
			node.TryGetValue("cometPersistentId", ref cometPersistentId);
			node.TryGetValue("scienceSubjectId", ref scienceSubjectId);
			node.TryGetValue("scienceDataCollected", ref scienceDataCollected);
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("cometName", cometName);
			node.AddValue("cometPersistentId", cometPersistentId);
			node.AddValue("scienceSubjectId", scienceSubjectId);
			node.AddValue("scienceDataCollected", scienceDataCollected);
		}

		protected override void OnRegister()
		{
			GameEvents.onVesselPersistentIdChanged.Add(VesselIdChanged);
			GameEvents.OnScienceRecieved.Add(OnScience);
			GameEvents.OnTriggeredDataTransmission.Add(OnTriggeredScience);
			GameEvents.onVesselRename.Add(OnVesselRename);
			GameEvents.OnExperimentDeployed.Add(OnExperimentDeployed);
		}

		protected override void OnUnregister()
		{
			GameEvents.onVesselPersistentIdChanged.Remove(VesselIdChanged);
			GameEvents.OnScienceRecieved.Remove(OnScience);
			GameEvents.OnTriggeredDataTransmission.Remove(OnTriggeredScience);
			GameEvents.onVesselRename.Remove(OnVesselRename);
			GameEvents.OnExperimentDeployed.Remove(OnExperimentDeployed);
		}

		private void OnExperimentDeployed(ScienceData experimentData)
		{
			if (experimentData.subjectID == scienceSubjectId)
			{
				scienceDataCollected = true;
			}
		}

		private void OnVesselRename(GameEvents.HostedFromToAction<Vessel, string> hostedFromTo)
		{
			if (!(hostedFromTo.host == null) && !scienceDataCollected && cometPersistentId == hostedFromTo.host.persistentId)
			{
				cometName = hostedFromTo.to;
				scienceSubjectId = "cometSample_" + hostedFromTo.host.Comet.typeName + "@SunInSpaceHigh_" + cometName;
			}
		}

		private void VesselIdChanged(uint oldId, uint newId)
		{
			if (cometPersistentId == oldId)
			{
				cometPersistentId = newId;
			}
		}

		private void OnScience(float science, ScienceSubject subject, ProtoVessel pv, bool reverseEngineered)
		{
			if (!reverseEngineered && subject.id == scienceSubjectId)
			{
				SetComplete();
				base.Root.Complete();
			}
		}

		private void OnTriggeredScience(ScienceData data, Vessel origin, bool xmitAborted)
		{
			if (!(data == null || xmitAborted) && !(data.dataAmount <= 0f) && data.subjectID == scienceSubjectId)
			{
				SetComplete();
				base.Root.Complete();
			}
		}
	}
}

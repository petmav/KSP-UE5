using System;
using Contracts;
using KSP.Localization;

namespace Expansions.Serenity.Contracts
{
	[Serializable]
	public class CollectDeployedScience : ContractParameter
	{
		protected CelestialBody targetBody;

		protected string subjectId = "";

		protected float sciencePercentage;

		public float SciencePercentage;

		protected float scienceCollected;

		public float ScienceCollected;

		protected string scienceTitle = "";

		public string ScienceTitle;

		public CelestialBody TargetBody => targetBody;

		public string SubjectId => subjectId;

		public CollectDeployedScience()
		{
		}

		public CollectDeployedScience(CelestialBody targetBody, string subjectId, string scienceTitle, float sciencePercentage)
		{
			this.targetBody = targetBody;
			this.subjectId = subjectId;
			this.scienceTitle = scienceTitle;
			this.sciencePercentage = sciencePercentage;
			scienceCollected = 0f;
		}

		protected override string GetHashString()
		{
			return SubjectId + " " + targetBody.name;
		}

		protected override string GetTitle()
		{
			if (base.Root.ContractState != Contract.State.Active && base.Root.ContractState != Contract.State.Cancelled && base.Root.ContractState != Contract.State.Completed && base.Root.ContractState != Contract.State.DeadlineExpired && base.Root.ContractState != Contract.State.Failed)
			{
				return Localizer.Format("#autoLOC_8002261", sciencePercentage.ToString("N0"), scienceTitle, targetBody.displayName);
			}
			return Localizer.Format("#autoLOC_8002260", scienceCollected.ToString("N0"), sciencePercentage.ToString("N0"), scienceTitle, targetBody.displayName);
		}

		protected override void OnLoad(ConfigNode node)
		{
			string value = "";
			if (node.TryGetValue("body", ref value))
			{
				targetBody = FlightGlobals.GetBodyByName(value);
			}
			node.TryGetValue("subjectId", ref subjectId);
			node.TryGetValue("scienceTitle", ref scienceTitle);
			node.TryGetValue("sciencePercentage", ref sciencePercentage);
			node.TryGetValue("scienceCollected", ref scienceCollected);
		}

		protected override void OnSave(ConfigNode node)
		{
			if (targetBody != null)
			{
				node.AddValue("body", targetBody.name);
			}
			node.AddValue("subjectId", subjectId);
			node.AddValue("scienceTitle", scienceTitle);
			node.AddValue("sciencePercentage", sciencePercentage);
			node.AddValue("scienceCollected", scienceCollected);
		}

		protected override void OnRegister()
		{
			GameEvents.OnScienceRecieved.Add(OnScience);
		}

		protected override void OnUnregister()
		{
			GameEvents.OnScienceRecieved.Remove(OnScience);
		}

		private void OnScience(float science, ScienceSubject subject, ProtoVessel pv, bool reverseEngineered)
		{
			if (ExpansionsLoader.IsExpansionInstalled("Serenity") && !reverseEngineered && subject.id.Substring(0, subject.id.IndexOf('@')) == subjectId && subject.IsFromBody(targetBody) && (subject.IsFromSituation(ExperimentSituations.SrfLanded) || subject.IsFromSituation(ExperimentSituations.SrfSplashed)))
			{
				scienceCollected = subject.science / subject.scienceCap * 100f;
				if (scienceCollected >= sciencePercentage)
				{
					SetComplete();
				}
				else
				{
					GameEvents.Contract.onParameterChange.Fire(base.Root, this);
				}
			}
		}
	}
}

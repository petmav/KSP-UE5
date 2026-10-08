using System;
using System.Collections.Generic;
using Contracts.Parameters;
using FinePrint;
using FinePrint.Utilities;
using KSP.Localization;
using UnityEngine;

namespace Contracts.Templates
{
	[Serializable]
	public class CollectScience : Contract
	{
		[SerializeField]
		protected CelestialBody targetBody;

		protected BodyLocation targetLocation;

		public CelestialBody TargetBody => targetBody;

		public BodyLocation TargetLocation => targetLocation;

		public override bool MeetRequirements()
		{
			if (!ProgressTracking.Instance.NodeComplete(FlightGlobals.GetHomeBodyName(), "Orbit"))
			{
				return false;
			}
			return true;
		}

		protected override List<CelestialBody> GetWeightBodies()
		{
			return new List<CelestialBody> { targetBody };
		}

		protected override string GetDescription()
		{
			if (targetLocation == BodyLocation.Space)
			{
				return TextGen.GenerateBackStories(GetType().Name, base.Agent.Title, targetBody.displayName, targetBody.displayName, base.MissionSeed, allowGenericIntroduction: true, allowGenericProblem: false, allowGenericConclusion: true);
			}
			return TextGen.GenerateBackStories(GetType().Name, base.Agent.Title, Localizer.Format("#autoLOC_272424"), targetBody.displayName + "Srf", base.MissionSeed, allowGenericIntroduction: true, allowGenericProblem: false, allowGenericConclusion: true);
		}

		protected override string GetSynopsys()
		{
			if (targetLocation == BodyLocation.Space)
			{
				return Localizer.Format("#autoLOC_272432", targetBody.displayName);
			}
			return Localizer.Format("#autoLOC_272436", targetBody.displayName);
		}

		protected override string GetTitle()
		{
			if (targetLocation == BodyLocation.Space)
			{
				return Localizer.Format("#autoLOC_272443", targetBody.displayName);
			}
			return Localizer.Format("#autoLOC_272445", targetBody.displayName);
		}

		protected override string MessageCompleted()
		{
			if (targetLocation == BodyLocation.Space)
			{
				return Localizer.Format("#autoLOC_272451", targetBody.displayName);
			}
			return Localizer.Format("#autoLOC_272453", targetBody.displayName);
		}

		protected override void OnLoad(ConfigNode node)
		{
			if (node.HasValue("body"))
			{
				targetBody = FlightGlobals.fetch.bodies[int.Parse(node.GetValue("body"))];
			}
			if (node.HasValue("location"))
			{
				targetLocation = (BodyLocation)Enum.Parse(typeof(BodyLocation), node.GetValue("location"));
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("body", targetBody.flightGlobalsIndex);
			node.AddValue("location", targetLocation.ToString());
		}

		protected override bool Generate()
		{
			if (ContractSystem.Instance.GetCurrentContracts<CollectScience>().Length >= ContractDefs.Research.MaximumExistent)
			{
				return false;
			}
			targetLocation = (BodyLocation)UnityEngine.Random.Range(0, 2);
			if (targetLocation == BodyLocation.Space)
			{
				return GenerateContract_Orbit();
			}
			return GenerateContract_Surface();
		}

		private bool GenerateContract_Orbit()
		{
			if (base.Prestige == ContractPrestige.Trivial)
			{
				List<CelestialBody> bodiesProgress = ProgressUtilities.GetBodiesProgress(ProgressType.ORBIT, progressComplete: true, (CelestialBody cb) => cb != Planetarium.fetch.Sun);
				if (bodiesProgress.Count == 0)
				{
					return false;
				}
				targetBody = WeightedBodyChoice(bodiesProgress);
				AddParameter(new Contracts.Parameters.CollectScience(targetBody, targetLocation));
				AddKeywords("Scientific", "Commercial");
			}
			else
			{
				if (base.Prestige != ContractPrestige.Significant)
				{
					return false;
				}
				List<CelestialBody> bodiesProgress2 = ProgressUtilities.GetBodiesProgress(ProgressType.ORBIT, bodyReached: true, progressComplete: false, (CelestialBody cb) => cb != Planetarium.fetch.Sun);
				if (bodiesProgress2.Count == 0)
				{
					return false;
				}
				targetBody = WeightedBodyChoice(bodiesProgress2);
				AddParameter(new Contracts.Parameters.CollectScience(targetBody, targetLocation));
				AddKeywords("Pioneer", "Scientific");
			}
			SetExpiry();
			SetDeadlineYears(5f, targetBody);
			SetFunds(ContractDefs.Research.Funds.BaseAdvance * 0.85f, ContractDefs.Research.Funds.BaseReward * 0.85f, ContractDefs.Research.Funds.BaseFailure * 0.85f, targetBody);
			SetReputation(ContractDefs.Research.Reputation.BaseReward * 0.85f, ContractDefs.Research.Reputation.BaseFailure * 0.85f);
			SetScience(ContractDefs.Research.Science.BaseReward * 0.85f);
			return true;
		}

		private bool GenerateContract_Surface()
		{
			if (base.Prestige == ContractPrestige.Trivial)
			{
				List<CelestialBody> bodiesProgress = ProgressUtilities.GetBodiesProgress(ProgressType.LANDING, progressComplete: true, (CelestialBody cb) => cb != Planetarium.fetch.Home && cb != Planetarium.fetch.Sun && cb.hasSolidSurface);
				if (bodiesProgress.Count == 0)
				{
					return false;
				}
				targetBody = WeightedBodyChoice(bodiesProgress);
				AddParameter(new Contracts.Parameters.CollectScience(targetBody, targetLocation));
				AddKeywords("Scientific", "Commercial");
			}
			else
			{
				if (base.Prestige != ContractPrestige.Significant)
				{
					return false;
				}
				List<CelestialBody> bodiesProgress2 = ProgressUtilities.GetBodiesProgress(ProgressType.LANDING, bodyReached: true, progressComplete: false, (CelestialBody cb) => cb != Planetarium.fetch.Home && cb != Planetarium.fetch.Sun && cb.hasSolidSurface);
				if (bodiesProgress2.Count == 0)
				{
					return false;
				}
				targetBody = WeightedBodyChoice(bodiesProgress2);
				AddParameter(new Contracts.Parameters.CollectScience(targetBody, targetLocation));
				AddKeywords("Scientific", "Commercial");
			}
			SetExpiry(ContractDefs.Research.Expire.MinimumExpireDays, ContractDefs.Research.Expire.MaximumExpireDays);
			SetDeadlineDays(ContractDefs.Research.Expire.DeadlineDays, targetBody);
			SetFunds(ContractDefs.Research.Funds.BaseAdvance * 1.15f, ContractDefs.Research.Funds.BaseReward * 1.15f, ContractDefs.Research.Funds.BaseFailure * 1.15f, targetBody);
			SetReputation(ContractDefs.Research.Reputation.BaseReward * 1.15f, ContractDefs.Research.Reputation.BaseFailure * 1.15f);
			SetScience(ContractDefs.Research.Science.BaseReward * 1.15f);
			return true;
		}
	}
}

using System.Collections.Generic;
using System.Globalization;
using Contracts;
using FinePrint.Contracts.Parameters;
using FinePrint.Utilities;
using KSP.Localization;
using UnityEngine;

namespace FinePrint.Contracts
{
	public class ARMContract : Contract
	{
		private CelestialBody targetBody;

		private string asteroidClass = "A";

		private bool isLanding;

		protected override bool Generate()
		{
			if (!ResearchAndDevelopment.ResearchedValidContractObjectives("Grapple"))
			{
				return false;
			}
			if (ContractSystem.Instance.GetCurrentContracts<ARMContract>().Length >= ContractDefs.ARM.MaximumExistent)
			{
				return false;
			}
			KSPRandom kSPRandom = new KSPRandom(base.MissionSeed);
			float num = 1f;
			float num2 = 1f;
			float num3 = 1f;
			if (prestige == ContractPrestige.Trivial)
			{
				asteroidClass = "A";
				targetBody = Planetarium.fetch.Home;
			}
			else if (prestige == ContractPrestige.Significant)
			{
				asteroidClass = (SystemUtilities.CoinFlip(kSPRandom) ? "B" : "C");
				List<CelestialBody> bodiesProgress = ProgressUtilities.GetBodiesProgress(ProgressType.ORBIT, progressComplete: true, (CelestialBody cb) => cb != Planetarium.fetch.Sun && cb.name != "Dres");
				if ((float)kSPRandom.Next(0, 100) < ContractDefs.ARM.SignificantSolarEjectionChance && ContractDefs.ARM.AllowSolarEjections)
				{
					bodiesProgress.Add(Planetarium.fetch.Sun);
				}
				if (bodiesProgress.Count == 0)
				{
					bodiesProgress.Add(Planetarium.fetch.Home);
				}
				targetBody = WeightedBodyChoice(bodiesProgress, kSPRandom);
			}
			else if (prestige == ContractPrestige.Exceptional)
			{
				asteroidClass = (SystemUtilities.CoinFlip(kSPRandom) ? "D" : "E");
				List<CelestialBody> bodiesProgress2 = ProgressUtilities.GetBodiesProgress(ProgressType.ORBIT, progressComplete: true, (CelestialBody cb) => cb != Planetarium.fetch.Sun && cb.name != "Dres");
				bodiesProgress2.AddRange(ProgressUtilities.GetNextUnreached(2, (CelestialBody cb) => cb != Planetarium.fetch.Sun && cb.name != "Dres"));
				if ((float)kSPRandom.Next(0, 100) < ContractDefs.ARM.ExceptionalSolarEjectionChance && ContractDefs.ARM.AllowSolarEjections)
				{
					bodiesProgress2.Add(Planetarium.fetch.Sun);
				}
				if (bodiesProgress2.Count == 0)
				{
					bodiesProgress2.Add(Planetarium.fetch.Home);
				}
				targetBody = WeightedBodyChoice(bodiesProgress2, kSPRandom);
			}
			if (targetBody == null)
			{
				targetBody = Planetarium.fetch.Home;
			}
			AddParameter(new AsteroidParameter(asteroidClass, forStation: false));
			if (targetBody == Planetarium.fetch.Sun)
			{
				AddKeywords("Pioneer");
				AddParameter(new LocationAndSituationParameter(targetBody, Vessel.Situations.ESCAPING, Localizer.Format("#autoLOC_7001016")));
				num *= ContractDefs.ARM.Funds.SolarEjectionMultiplier;
				num2 *= ContractDefs.ARM.Science.SolarEjectionMultiplier;
				num3 *= ContractDefs.ARM.Reputation.SolarEjectionMultiplier;
			}
			else if (targetBody == Planetarium.fetch.Home && (float)kSPRandom.Next(0, 101) < ContractDefs.ARM.HomeLandingChance && ContractDefs.ARM.AllowHomeLandings)
			{
				isLanding = true;
				AddParameter(new LocationAndSituationParameter(targetBody, Vessel.Situations.LANDED, Localizer.Format("#autoLOC_7001016")));
			}
			else
			{
				AddParameter(new LocationAndSituationParameter(targetBody, Vessel.Situations.ORBITING, Localizer.Format("#autoLOC_7001016")));
			}
			float num4 = 1f;
			switch (asteroidClass)
			{
			case "E":
				num4 = 2f;
				break;
			case "D":
				num4 = 1.75f;
				break;
			case "C":
				num4 = 1.5f;
				break;
			case "B":
				num4 = 1.25f;
				break;
			}
			num *= num4;
			num2 *= num4;
			num3 *= num4;
			AddKeywords("Scientific");
			SetExpiry(ContractDefs.ARM.Expire.MinimumExpireDays, ContractDefs.ARM.Expire.MaximumExpireDays);
			SetDeadlineDays(ContractDefs.ARM.Expire.DeadlineDays, targetBody);
			SetFunds(Mathf.Round(ContractDefs.ARM.Funds.BaseAdvance * num), Mathf.Round(ContractDefs.ARM.Funds.BaseReward * num), Mathf.Round(ContractDefs.ARM.Funds.BaseFailure * num), targetBody);
			SetScience(Mathf.Round(ContractDefs.ARM.Science.BaseReward * num2));
			SetReputation(Mathf.Round(ContractDefs.ARM.Reputation.BaseReward * num3), Mathf.Round(ContractDefs.ARM.Reputation.BaseFailure * num3));
			if (ContractSystem.Instance.AnyCurrentContracts((ARMContract contract) => contract.targetBody == targetBody && contract.asteroidClass == asteroidClass))
			{
				return false;
			}
			AddParameter(new StabilityParameter(10f));
			return true;
		}

		public override bool CanBeCancelled()
		{
			return true;
		}

		public override bool CanBeDeclined()
		{
			return true;
		}

		protected override string GetHashString()
		{
			return SystemUtilities.SuperSeed(this).ToString(CultureInfo.InvariantCulture);
		}

		protected override string GetTitle()
		{
			if (targetBody != Planetarium.fetch.Sun)
			{
				if (isLanding)
				{
					return Localizer.Format("#autoLOC_278134", asteroidClass, targetBody.displayName);
				}
				return Localizer.Format("#autoLOC_278136", asteroidClass, targetBody.displayName);
			}
			return Localizer.Format("#autoLOC_278139", asteroidClass);
		}

		protected override string GetDescription()
		{
			return TextGen.GenerateBackStories(GetType().Name, base.Agent.Title, targetBody.displayName, Localizer.Format("#autoLOC_7001015"), base.MissionSeed, allowGenericIntroduction: true, allowGenericProblem: false, allowGenericConclusion: true);
		}

		protected override string GetSynopsys()
		{
			KSPRandom kSPRandom = new KSPRandom(base.MissionSeed);
			if (targetBody != Planetarium.fetch.Sun)
			{
				if (isLanding)
				{
					return Localizer.Format("#autoLOC_278155", asteroidClass, targetBody.displayName);
				}
				switch (kSPRandom.Next(0, 3))
				{
				default:
					return Localizer.Format("#autoLOC_278166", Planetarium.fetch.Home.displayName, asteroidClass, targetBody.displayName);
				case 1:
					return Localizer.Format("#autoLOC_278164", asteroidClass, targetBody.displayName);
				case 0:
					return Localizer.Format("#autoLOC_278162", asteroidClass, targetBody.displayName);
				}
			}
			switch (kSPRandom.Next(0, 3))
			{
			default:
				return Localizer.Format("#autoLOC_278179", asteroidClass);
			case 1:
				return Localizer.Format("#autoLOC_278177", asteroidClass, Planetarium.fetch.Home.displayName);
			case 0:
				return Localizer.Format("#autoLOC_278175", asteroidClass);
			}
		}

		protected override string MessageCompleted()
		{
			KSPRandom kSPRandom = new KSPRandom(base.MissionSeed);
			if (targetBody != Planetarium.fetch.Sun)
			{
				return Localizer.Format("#autoLOC_278189", targetBody.displayName);
			}
			switch (kSPRandom.Next(0, 3))
			{
			default:
				return Localizer.Format("#autoLOC_278199");
			case 1:
				return Localizer.Format("#autoLOC_278197");
			case 0:
				return Localizer.Format("#autoLOC_278195");
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("targetBody", targetBody.flightGlobalsIndex);
			node.AddValue("asteroidClass", asteroidClass);
			node.AddValue("isLanding", isLanding);
		}

		protected override void OnLoad(ConfigNode node)
		{
			SystemUtilities.LoadNode(node, "ARMContract", "targetBody", ref targetBody, Planetarium.fetch.Home);
			SystemUtilities.LoadNode(node, "ARMContract", "asteroidClass", ref asteroidClass, "A");
			SystemUtilities.LoadNode(node, "ARMContract", "isLanding", ref isLanding, defaultValue: false);
		}

		public override bool MeetRequirements()
		{
			if (!ProgressTracking.Instance.NodeComplete(Planetarium.fetch.Sun.name, "Orbit"))
			{
				return false;
			}
			if (!GameVariables.Instance.UnlockedSpaceObjectDiscovery(ScenarioUpgradeableFacilities.GetFacilityLevel(SpaceCenterFacility.TrackingStation)))
			{
				return false;
			}
			return true;
		}

		protected override List<CelestialBody> GetWeightBodies()
		{
			return new List<CelestialBody> { targetBody };
		}
	}
}

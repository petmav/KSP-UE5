using FinePrint.Utilities;
using KSP.Localization;
using UnityEngine;
using Upgradeables;

namespace KSPAchievements
{
	public class TowerBuzz : ProgressNode
	{
		private bool active;

		private double latitude;

		private double longitude;

		private VesselRef firstVessel;

		private CrewRef firstCrew;

		public TowerBuzz()
			: base("TowerBuzz", startReached: false)
		{
			OnDeploy = delegate
			{
				GameEvents.onFlightReady.Add(OnFlightReady);
				GameEvents.OnFlightGlobalsReady.Add(OnFlightGlobalsReady);
				GameEvents.OnKSCFacilityUpgraded.Add(OnFacilityUpgrade);
			};
			OnStow = delegate
			{
				GameEvents.onFlightReady.Remove(OnFlightReady);
				GameEvents.OnFlightGlobalsReady.Remove(OnFlightGlobalsReady);
				GameEvents.OnKSCFacilityUpgraded.Remove(OnFacilityUpgrade);
			};
			firstVessel = new VesselRef();
			firstCrew = new CrewRef();
		}

		private void OnFlightReady()
		{
			Initialize();
		}

		private void OnFlightGlobalsReady(bool ready)
		{
			if (ready)
			{
				Initialize();
			}
		}

		private void OnFacilityUpgrade(UpgradeableFacility facility, int level)
		{
			if (facility.name == "SpaceplaneHangar")
			{
				Initialize();
			}
		}

		private void iterateVessels(Vessel v)
		{
			if (active && !(v == null) && !(v != FlightGlobals.ActiveVessel) && v.situation == Vessel.Situations.FLYING && v.isCommandable && v.DiscoveryInfo.Level == DiscoveryLevels.Owned && !(v.altitude > 200.0) && v.srfSpeed >= 200.0)
			{
				CelestialBody homeBody = FlightGlobals.GetHomeBody();
				if ((!(v.mainBody != null) || !(v.mainBody != homeBody)) && !(CelestialUtilities.GreatCircleDistance(homeBody, latitude, longitude, v.latitude, v.longitude) > 50.0) && !base.IsComplete)
				{
					firstVessel = VesselRef.FromVessel(v);
					firstCrew = CrewRef.FromVessel(v);
					Complete();
					AwardProgressStandard(Localizer.Format("#autoLOC_298640"), ProgressType.STUNT, homeBody);
					Terminate();
				}
			}
		}

		protected override void OnLoad(ConfigNode node)
		{
			if (node.HasNode("vessel"))
			{
				firstVessel.Load(node.GetNode("vessel"));
			}
			if (node.HasNode("crew"))
			{
				firstCrew.Load(node.GetNode("crew"));
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			firstVessel.Save(node.AddNode("vessel"));
			if (firstCrew.HasAny)
			{
				firstCrew.Save(node.AddNode("crew"));
			}
		}

		private void SetActive(bool active)
		{
			if (active && OnIterateVessels == null)
			{
				OnIterateVessels = iterateVessels;
			}
			else if (!active && OnIterateVessels != null)
			{
				OnIterateVessels = null;
			}
			this.active = active;
		}

		private void Initialize()
		{
			if (base.IsComplete)
			{
				latitude = 0.0;
				longitude = 0.0;
				Terminate();
				return;
			}
			switch (Mathf.RoundToInt(ScenarioUpgradeableFacilities.GetFacilityLevel(SpaceCenterFacility.SpaceplaneHangar) * (float)ScenarioUpgradeableFacilities.GetFacilityLevelCount(SpaceCenterFacility.SpaceplaneHangar)))
			{
			case 0:
				latitude = 0.0;
				longitude = 0.0;
				SetActive(active: false);
				break;
			case 1:
				latitude = -0.0660831849602887;
				longitude = 285.366569132555;
				SetActive(active: true);
				break;
			case 2:
				latitude = -0.0627680294327377;
				longitude = 285.36674754833;
				SetActive(active: true);
				break;
			}
		}

		private void Terminate()
		{
			active = false;
			OnStow();
			OnIterateVessels = null;
			OnDeploy = null;
			OnStow = null;
		}
	}
}

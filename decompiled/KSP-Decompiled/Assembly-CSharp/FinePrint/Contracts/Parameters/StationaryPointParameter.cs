using System;
using System.Globalization;
using Contracts;
using FinePrint.Utilities;
using KSP.Localization;

namespace FinePrint.Contracts.Parameters
{
	public class StationaryPointParameter : ContractParameter
	{
		private Waypoint wp;

		private CelestialBody targetBody;

		private bool submittedWaypoint;

		private double longitude;

		private double deviation;

		private int successCounter;

		public StationaryPointParameter()
		{
			targetBody = Planetarium.fetch.Home;
			longitude = 0.0;
			wp = new Waypoint();
			submittedWaypoint = false;
			successCounter = 0;
			deviation = ContractDefs.Satellite.TrivialDeviation;
		}

		public StationaryPointParameter(CelestialBody targetBody, double longitude, double deviation)
		{
			this.targetBody = targetBody;
			wp = new Waypoint();
			submittedWaypoint = false;
			successCounter = 0;
			this.longitude = longitude;
			this.deviation = deviation;
		}

		protected override string GetHashString()
		{
			return SystemUtilities.SuperSeed(base.Root).ToString(CultureInfo.InvariantCulture) + base.ID;
		}

		protected override string GetTitle()
		{
			return Localizer.Format("#autoLOC_285514", StringUtilities.GenerateSiteName(SystemUtilities.SuperSeed(base.Root), targetBody, landLocked: true));
		}

		protected override string GetMessageComplete()
		{
			return Localizer.Format("#autoLOC_285519", StringUtilities.GenerateSiteName(SystemUtilities.SuperSeed(base.Root), targetBody, landLocked: true));
		}

		protected override void OnRegister()
		{
			base.DisableOnStateChange = false;
		}

		protected override void OnUnregister()
		{
			if (submittedWaypoint)
			{
				WaypointManager.RemoveWaypoint(wp);
			}
		}

		protected override void OnReset()
		{
			SetIncomplete();
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("targetBody", targetBody.flightGlobalsIndex);
			node.AddValue("longitude", longitude);
			node.AddValue("deviation", deviation);
		}

		protected override void OnLoad(ConfigNode node)
		{
			SystemUtilities.LoadNode(node, "StationaryPointParameter", "targetBody", ref targetBody, Planetarium.fetch.Home);
			SystemUtilities.LoadNode(node, "StationaryPointParameter", "longitude", ref longitude, 0.0);
			SystemUtilities.LoadNode(node, "StationaryPointParameter", "deviation", ref deviation, ContractDefs.Satellite.TrivialDeviation);
			if (HighLogic.LoadedSceneIsFlight && base.Root.ContractState == Contract.State.Active)
			{
				wp.celestialName = targetBody.GetName();
				wp.latitude = 0.0;
				wp.longitude = longitude;
				wp.seed = SystemUtilities.SuperSeed(base.Root);
				wp.index = 0;
				wp.landLocked = true;
				wp.setName(uniqueSites: false);
				wp.id = "dish";
				wp.altitude = 0.0;
				wp.isOnSurface = true;
				wp.isNavigatable = true;
				wp.contractReference = base.Root;
				wp.SetFadeRange();
				WaypointManager.AddWaypoint(wp);
				submittedWaypoint = true;
			}
			if (HighLogic.LoadedScene == GameScenes.TRACKSTATION && base.State == ParameterState.Incomplete && (base.Root.ContractState == Contract.State.Active || (!base.Root.IsFinished() && ContractDefs.DisplayOfferedOrbits)))
			{
				wp.celestialName = targetBody.GetName();
				wp.latitude = 0.0;
				wp.longitude = longitude;
				wp.seed = SystemUtilities.SuperSeed(base.Root);
				wp.index = 0;
				wp.landLocked = true;
				wp.setName(uniqueSites: false);
				wp.id = "dish";
				wp.altitude = 0.0;
				wp.isOnSurface = true;
				wp.isNavigatable = true;
				wp.contractReference = base.Root;
				wp.SetFadeRange();
				WaypointManager.AddWaypoint(wp);
				submittedWaypoint = true;
			}
		}

		protected override void OnUpdate()
		{
			if (!SystemUtilities.FlightIsReady(base.Root.ContractState, Contract.State.Active, checkVessel: true))
			{
				return;
			}
			Vessel activeVessel = FlightGlobals.ActiveVessel;
			if (!submittedWaypoint || activeVessel.mainBody != targetBody)
			{
				return;
			}
			bool flag = 180.0 - Math.Abs(Math.Abs(activeVessel.longitude - longitude) - 180.0) <= deviation / 100.0 * 360.0;
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

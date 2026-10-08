using System.Globalization;
using Contracts;
using FinePrint.Utilities;
using KSP.Localization;

namespace FinePrint.Contracts.Parameters
{
	public class RoverWayPointParameter : WaypointParameter
	{
		protected Vessel roverVessel;

		protected uint roverVslId;

		protected Waypoint startwp;

		protected Waypoint endwp;

		protected double craftStartLatitude;

		protected double craftStartLongitude;

		protected double craftEndLatitude;

		protected double craftEndLongitude;

		protected bool submittedWaypoint;

		protected int successCounter;

		protected bool outerWarning;

		public Vessel RoverVessel => roverVessel;

		public uint RoverVslId => roverVslId;

		public Waypoint Startwp => startwp;

		public Waypoint Endwp => endwp;

		public double CraftStartLatitude => craftStartLatitude;

		public double CraftStartLongitude => craftStartLongitude;

		public double CraftEndLatitude => craftEndLatitude;

		public double CraftEndLongitude => craftEndLongitude;

		public RoverWayPointParameter()
		{
			roverVessel = null;
			roverVslId = 0u;
			TargetBody = Planetarium.fetch.Home;
			startwp = new Waypoint();
			endwp = new Waypoint();
			craftStartLongitude = 0.0;
			craftStartLatitude = 0.0;
			craftEndLatitude = 0.0;
			craftEndLongitude = 0.0;
			TargetBody = null;
		}

		public RoverWayPointParameter(double CraftStartLatitude, double CraftStartLongitude, double CraftEndLatitude, double CraftEndLongitude, CelestialBody targetBody)
		{
			roverVessel = null;
			roverVslId = 0u;
			startwp = new Waypoint();
			endwp = new Waypoint();
			endwp.setName();
			craftStartLongitude = CraftStartLongitude;
			craftStartLatitude = CraftStartLatitude;
			craftEndLatitude = CraftEndLatitude;
			craftEndLongitude = CraftEndLongitude;
			TargetBody = targetBody;
		}

		protected override string GetHashString()
		{
			return SystemUtilities.SuperSeed(base.Root).ToString(CultureInfo.InvariantCulture) + base.ID;
		}

		protected override string GetTitle()
		{
			return Localizer.Format("#autoLOC_6002560");
		}

		protected override string GetMessageComplete()
		{
			return Localizer.Format("#autoLOC_6002561");
		}

		protected override string GetNotes()
		{
			if (!base.Root.IsFinished() && !(roverVessel == null) && !(TargetBody == null))
			{
				return Localizer.Format("#autoLOC_6002562", roverVessel.vesselName, endwp.name, TargetBody.displayName);
			}
			return null;
		}

		protected override void OnRegister()
		{
			base.DisableOnStateChange = false;
			GameEvents.onVesselPersistentIdChanged.Add(VesselIdChanged);
		}

		protected override void OnUnregister()
		{
			CleanupWaypoints();
			GameEvents.onVesselPersistentIdChanged.Remove(VesselIdChanged);
		}

		protected override void OnReset()
		{
			SetIncomplete();
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("bodyName", TargetBody.bodyName);
			node.AddValue("roverVslId", roverVslId);
			node.AddValue("craftStartLat", CraftStartLatitude);
			node.AddValue("craftStartLon", CraftStartLongitude);
			node.AddValue("craftEndLat", CraftEndLatitude);
			node.AddValue("craftEndLon", CraftEndLongitude);
			if (endwp != null)
			{
				node.AddValue("endWPName", endwp.name);
			}
		}

		protected override void OnLoad(ConfigNode node)
		{
			string value = "";
			if (node.TryGetValue("bodyName", ref value))
			{
				TargetBody = FlightGlobals.GetBodyByName(value);
			}
			node.TryGetValue("roverVslId", ref roverVslId);
			FlightGlobals.FindVessel(roverVslId, out roverVessel);
			node.TryGetValue("craftStartLat", ref craftStartLatitude);
			node.TryGetValue("craftStartLon", ref craftStartLongitude);
			node.TryGetValue("craftEndLat", ref craftEndLatitude);
			node.TryGetValue("craftEndLon", ref craftEndLongitude);
			if (endwp != null)
			{
				node.TryGetValue("endWPName", ref endwp.name);
			}
			if (HighLogic.LoadedSceneIsFlight && base.Root.ContractState == Contract.State.Active && endwp != null)
			{
				endwp.celestialName = TargetBody.GetName();
				endwp.latitude = CraftEndLatitude;
				endwp.longitude = CraftEndLongitude;
				endwp.seed = SystemUtilities.SuperSeed(base.Root);
				endwp.index = 1;
				endwp.landLocked = false;
				endwp.id = "vesselEndWP";
				endwp.iconSize = 32;
				endwp.altitude = 0.0;
				endwp.isOnSurface = true;
				endwp.isNavigatable = true;
				endwp.enableTooltip = true;
				endwp.enableMarker = true;
				endwp.blocksInput = false;
				endwp.contractReference = base.Root;
				endwp.SetFadeRange();
				WaypointManager.AddWaypoint(endwp);
				submittedWaypoint = true;
			}
			if (HighLogic.LoadedScene == GameScenes.TRACKSTATION)
			{
				if (base.Root.ContractState == Contract.State.Offered)
				{
					startwp.celestialName = TargetBody.GetName();
					startwp.latitude = CraftStartLatitude;
					startwp.longitude = CraftStartLongitude;
					startwp.seed = SystemUtilities.SuperSeed(base.Root);
					startwp.index = 0;
					startwp.landLocked = false;
					startwp.name = Localizer.Format("#autoLOC_6002563");
					startwp.id = "vesselStartWP";
					startwp.iconSize = 32;
					startwp.altitude = 0.0;
					startwp.isOnSurface = true;
					startwp.isNavigatable = false;
					startwp.enableTooltip = true;
					startwp.enableMarker = true;
					startwp.blocksInput = false;
					startwp.contractReference = base.Root;
					startwp.SetFadeRange();
					WaypointManager.AddWaypoint(startwp);
				}
				if (base.Root.ContractState == Contract.State.Active || (!base.Root.IsFinished() && ContractDefs.DisplayOfferedWaypoints && endwp != null))
				{
					endwp.celestialName = TargetBody.GetName();
					endwp.latitude = CraftEndLatitude;
					endwp.longitude = CraftEndLongitude;
					endwp.seed = SystemUtilities.SuperSeed(base.Root);
					endwp.index = 1;
					endwp.landLocked = false;
					endwp.id = "vesselEndWP";
					endwp.iconSize = 32;
					endwp.altitude = 0.0;
					endwp.isOnSurface = true;
					endwp.isNavigatable = true;
					endwp.enableTooltip = true;
					endwp.enableMarker = true;
					endwp.blocksInput = false;
					endwp.contractReference = base.Root;
					endwp.SetFadeRange();
					WaypointManager.AddWaypoint(endwp);
					submittedWaypoint = true;
				}
			}
		}

		protected override void OnUpdate()
		{
			if (!SystemUtilities.FlightIsReady(base.Root.ContractState, Contract.State.Active, checkVessel: true) || roverVessel == null || base.State != 0 || !submittedWaypoint || !(roverVessel.mainBody == TargetBody) || !(WaypointManager.Instance() != null))
			{
				return;
			}
			float num = WaypointManager.Instance().LateralDistanceToVessel(endwp, roverVessel);
			if (num > ContractDefs.RoverConstruction.WayPointTriggerRange && outerWarning)
			{
				string text = endwp.name;
				if (endwp.isClustered)
				{
					text = text + " " + StringUtilities.IntegerToGreek(endwp.index);
				}
				outerWarning = false;
				ScreenMessages.PostScreenMessage(Localizer.Format("#autoLOC_285861", text), 5f, ScreenMessageStyle.UPPER_CENTER);
			}
			if (num <= ContractDefs.RoverConstruction.WayPointTriggerRange && !outerWarning)
			{
				string text2 = endwp.name;
				if (endwp.isClustered)
				{
					text2 = text2 + " " + StringUtilities.IntegerToGreek(endwp.index);
				}
				outerWarning = true;
				ScreenMessages.PostScreenMessage(Localizer.Format("#autoLOC_285872", text2), 5f, ScreenMessageStyle.UPPER_CENTER);
			}
			NavWaypoint fetch = NavWaypoint.fetch;
			if (fetch != null && fetch.IsActive && fetch.IsUsing(endwp))
			{
				if (num <= ContractDefs.RoverConstruction.WayPointTriggerRange)
				{
					if (!fetch.IsBlinking)
					{
						fetch.IsBlinking = true;
					}
				}
				else if (fetch.IsBlinking)
				{
					fetch.IsBlinking = false;
				}
			}
			if (num <= ContractDefs.RoverConstruction.WayPointTriggerRange)
			{
				successCounter++;
				if (successCounter >= 5)
				{
					SetComplete();
				}
			}
		}

		public override void UpdateWaypoints(bool focus)
		{
			if (!(roverVessel == null) && !base.Root.IsFinished())
			{
				startwp.celestialName = TargetBody.GetName();
				endwp.celestialName = startwp.celestialName;
				startwp.name = (roverVessel.loaded ? roverVessel.GetDisplayName() : roverVessel.protoVessel.GetDisplayName());
				startwp.latitude = CraftStartLatitude;
				startwp.longitude = CraftStartLongitude;
				startwp.altitude = 0.0;
				startwp.isOnSurface = true;
				endwp.latitude = CraftEndLatitude;
				endwp.longitude = CraftEndLongitude;
				endwp.altitude = 0.0;
				endwp.isOnSurface = true;
			}
		}

		public void UpdateRoverInfo(Vessel vsl)
		{
			roverVessel = vsl;
			roverVslId = vsl.persistentId;
		}

		public override void CleanupWaypoints()
		{
			if (submittedWaypoint)
			{
				WaypointManager.RemoveWaypoint(startwp);
				WaypointManager.RemoveWaypoint(endwp);
				submittedWaypoint = false;
			}
		}

		private void VesselIdChanged(uint oldId, uint newId)
		{
			if (roverVslId == oldId)
			{
				roverVslId = newId;
			}
		}
	}
}

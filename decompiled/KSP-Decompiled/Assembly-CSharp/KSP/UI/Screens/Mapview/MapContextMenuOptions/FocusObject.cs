using KSP.Localization;
using PreFlightTests;

namespace KSP.UI.Screens.Mapview.MapContextMenuOptions
{
	public class FocusObject : MapContextMenuOption
	{
		public enum FocusMode
		{
			OwnedVessel,
			UnownedVessel,
			CelestialBody
		}

		private Vessel vessel;

		private CelestialBody celestialBody;

		public FocusObject(OrbitDriver tgt)
			: base("Focus")
		{
			vessel = tgt.vessel;
			celestialBody = tgt.celestialBody;
		}

		private FocusMode GetMode()
		{
			if ((bool)vessel)
			{
				if (vessel.DiscoveryInfo.Level == DiscoveryLevels.Owned)
				{
					return FocusMode.OwnedVessel;
				}
				return FocusMode.UnownedVessel;
			}
			return FocusMode.CelestialBody;
		}

		protected override void OnSelect()
		{
			switch (GetMode())
			{
			case FocusMode.OwnedVessel:
				if (HighLogic.CurrentGame.Parameters.Flight.CanSwitchVesselsFar)
				{
					if (FlightGlobals.SetActiveVessel(vessel))
					{
						MapView.ExitMapView();
						FlightInputHandler.SetNeutralControls();
					}
				}
				else
				{
					ScreenMessages.PostScreenMessage(Localizer.Format("#autoLOC_465647"), 5f, ScreenMessageStyle.UPPER_CENTER);
				}
				break;
			case FocusMode.UnownedVessel:
				TryViewInTrackingStation();
				break;
			case FocusMode.CelestialBody:
				PlanetariumCamera.fetch.SetTarget(celestialBody.MapObject);
				break;
			}
		}

		protected override bool OnCheckEnabled(out string fbText)
		{
			switch (GetMode())
			{
			default:
				fbText = Localizer.Format("#autoLOC_465671");
				break;
			case FocusMode.UnownedVessel:
				fbText = Localizer.Format("#autoLOC_465675");
				break;
			case FocusMode.CelestialBody:
				fbText = Localizer.Format("#autoLOC_465679");
				break;
			}
			return true;
		}

		public override bool CheckAvailable()
		{
			return true;
		}

		private void TryViewInTrackingStation()
		{
			InputLockManager.SetControlLock("OrbitTargeterCheckingTrackingStationState");
			PreFlightCheck preFlightCheck = new PreFlightCheck(onTrackingStationProceed, onTrackingStationDismiss);
			preFlightCheck.AddTest(new FacilityOperational("TrackingStation", "Tracking Station"));
			preFlightCheck.RunTests();
		}

		private void onTrackingStationProceed()
		{
			MapView.ExitMapView();
			SpaceTracking.GoToAndFocusVessel(vessel);
			onTrackingStationDismiss();
		}

		private void onTrackingStationDismiss()
		{
			InputLockManager.RemoveControlLock("OrbitTargeterCheckingTrackingStationState");
		}
	}
}

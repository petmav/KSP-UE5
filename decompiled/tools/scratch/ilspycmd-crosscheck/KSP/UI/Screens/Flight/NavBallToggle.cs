using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.Flight
{
	public class NavBallToggle : MonoBehaviour
	{
		public UIPanelTransitionToggle panel;

		public Button overrideButton;

		private bool maneuverModeActive;

		public static NavBallToggle Instance;

		public bool ManeuverModeActive => maneuverModeActive;

		public event Callback<bool> onManeuverModeToggle = delegate
		{
		};

		protected void Awake()
		{
			Instance = this;
			overrideButton.onClick.AddListener(TogglePanel);
			GameEvents.OnMapEntered.Add(OnMapEntered);
			GameEvents.OnMapExited.Add(OnMapExited);
			overrideButton.onClick.AddListener(OnNavBallToggle);
		}

		protected void OnDestroy()
		{
			GameEvents.OnMapEntered.Remove(OnMapEntered);
			if (Instance != null && Instance == this)
			{
				Instance = null;
			}
		}

		protected void TogglePanel()
		{
			if (panel.expanded)
			{
				panel.Collapse();
			}
			else if (panel.collapsed)
			{
				panel.Expand();
			}
		}

		protected void OnMapEntered()
		{
			if (GameSettings.AUTOHIDE_NAVBALL)
			{
				DisableManeuverMode();
				panel.Collapse();
			}
			else
			{
				panel.Expand();
				EnableManeuverMode();
			}
		}

		protected void OnMapExited()
		{
			if (panel != null)
			{
				panel.Expand();
			}
		}

		protected void Update()
		{
			if (InputLockManager.IsUnlocked(ControlTypes.UI_DIALOGS) && GameSettings.NAVBALL_TOGGLE.GetKeyUp())
			{
				TogglePanel();
				if (MapView.fetch != null)
				{
					OnNavBallToggle();
				}
			}
		}

		public void OnNavBallToggle()
		{
			if (maneuverModeActive)
			{
				DisableManeuverMode();
			}
			else
			{
				EnableManeuverMode();
			}
		}

		private void EnableManeuverMode()
		{
			if (MapView.MapIsEnabled)
			{
				maneuverModeActive = true;
				if (FlightUIModeController.Instance.Mode != 0 && FlightUIModeController.Instance.Mode != FlightUIMode.DOCKING)
				{
					InputLockManager.SetControlLock(ControlTypes.GROUPS_ALL | ControlTypes.STAGING | ControlTypes.CAMERAMODES, "MapView");
					InputLockManager.RemoveControlLock("stageDockInMap");
				}
				else
				{
					InputLockManager.SetControlLock(ControlTypes.None, "stageDockInMap");
					InputLockManager.RemoveControlLock("MapView");
				}
				Debug.Log("Maneuver Mode enabled");
				if (this.onManeuverModeToggle != null)
				{
					this.onManeuverModeToggle(maneuverModeActive);
				}
			}
		}

		private void DisableManeuverMode()
		{
			maneuverModeActive = false;
			if (MapView.MapIsEnabled)
			{
				InputLockManager.SetControlLock(~(ControlTypes.UI | ControlTypes.MAP | ControlTypes.PAUSE | ControlTypes.MISC | ControlTypes.CAMERACONTROLS | ControlTypes.TIMEWARP | ControlTypes.QUICKSAVE | ControlTypes.QUICKLOAD | ControlTypes.VESSEL_SWITCHING | ControlTypes.EDITOR_EDIT_NAME_FIELDS | ControlTypes.GUI | ControlTypes.FLIGHTUIMODE | ControlTypes.TARGETING | ControlTypes.MANNODE_ADDEDIT | ControlTypes.MANNODE_DELETE | ControlTypes.THROTTLE_CUT_MAX), "MapView");
				InputLockManager.RemoveControlLock("stageDockInMap");
				Debug.Log("Maneuver Mode disabled");
				if (this.onManeuverModeToggle != null)
				{
					this.onManeuverModeToggle(maneuverModeActive);
				}
			}
		}
	}
}

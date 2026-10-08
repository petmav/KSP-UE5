using TMPro;
using UnityEngine;

namespace KSP.UI.Screens
{
	public class ActionGroupsApp : UIApp
	{
		[SerializeField]
		private TMP_Text overrideGroupTextPrefab;

		[SerializeField]
		private GenericAppFrame appFramePrefab;

		private GenericAppFrame appFrame;

		private ActionGroupsPanel agPanel;

		private RectTransform appFrameRectTransform;

		public static ActionGroupsApp Instance { get; private set; }

		public TMP_Text overrideGroupText { get; private set; }

		protected override bool OnAppAboutToStart()
		{
			return true;
		}

		protected override ApplicationLauncher.AppScenes GetAppScenes()
		{
			SpaceCenterFacility spaceCenterFacility = SpaceCenterFacility.VehicleAssemblyBuilding;
			if (HighLogic.LoadedScene == GameScenes.EDITOR)
			{
				spaceCenterFacility = EditorDriver.editorFacility.ToFacility();
			}
			else if (HighLogic.LoadedScene == GameScenes.FLIGHT)
			{
				if (ScenarioUpgradeableFacilities.IsLaunchPad(FlightGlobals.ActiveVessel.launchedFrom))
				{
					spaceCenterFacility = SpaceCenterFacility.VehicleAssemblyBuilding;
				}
				else if (ScenarioUpgradeableFacilities.IsRunway(FlightGlobals.ActiveVessel.launchedFrom))
				{
					spaceCenterFacility = SpaceCenterFacility.SpaceplaneHangar;
				}
			}
			if (!GameVariables.Instance.UnlockedActionGroupsStock(ScenarioUpgradeableFacilities.GetFacilityLevel(spaceCenterFacility), spaceCenterFacility == SpaceCenterFacility.VehicleAssemblyBuilding))
			{
				return ApplicationLauncher.AppScenes.NEVER;
			}
			return ApplicationLauncher.AppScenes.FLIGHT | ApplicationLauncher.AppScenes.MAPVIEW;
		}

		protected override Vector3 GetAppScreenPos(Vector3 defaultAnchorPos)
		{
			return defaultAnchorPos;
		}

		protected override void OnAppInitialized()
		{
			Debug.Log("[ActionGroupsApp] OnAppStarted(): id: " + GetInstanceID());
			if (Instance != null)
			{
				Debug.Log("ActionGroupsApp already exist, destroying this instance");
				Object.DestroyImmediate(base.gameObject);
				return;
			}
			Instance = this;
			GameEvents.onVesselWasModified.Add(OnVesselWasModified);
			GameEvents.onVesselChange.Add(OnVesselChanged);
			GameEvents.onVesselDestroy.Add(OnVesselDestroy);
			GameEvents.onVesselGoOffRails.Add(OnVesselGoOffRails);
			GameEvents.OnVesselOverrideGroupChanged.Add(OnVesselOverrideGroupChanged);
			appFrame = Object.Instantiate(appFramePrefab);
			appFrame.transform.SetParent(base.transform, worldPositionStays: false);
			appFrame.transform.localPosition = Vector3.zero;
			appFrame.anchorToAppButton = true;
			appFrameRectTransform = appFrame.transform as RectTransform;
			UpdateOverrideGroupText();
			appFrame.Setup(base.appLauncherButton, base.name, base.name, 266, 48, scaleHeightToContainList: false);
			appFrame.AddGlobalInputDelegate(base.MouseInput_PointerEnter, base.MouseInput_PointerExit);
			ApplicationLauncher.Instance.AddOnRepositionCallback(Reposition);
			ReadjustPosition();
			agPanel = appFrame.GetComponent<ActionGroupsPanel>();
			HideApp();
		}

		protected override void OnAppDestroy()
		{
			if (appFrame != null)
			{
				if ((bool)ApplicationLauncher.Instance)
				{
					ApplicationLauncher.Instance.RemoveOnRepositionCallback(Reposition);
				}
				appFrame.gameObject.DestroyGameObject();
			}
			GameEvents.onVesselWasModified.Remove(OnVesselWasModified);
			GameEvents.onVesselChange.Remove(OnVesselChanged);
			GameEvents.onVesselDestroy.Remove(OnVesselDestroy);
			GameEvents.onVesselGoOffRails.Remove(OnVesselGoOffRails);
			GameEvents.OnVesselOverrideGroupChanged.Remove(OnVesselOverrideGroupChanged);
			if (Instance != null && Instance == this)
			{
				Instance = null;
			}
		}

		protected override void Reposition()
		{
			appFrame.Reposition();
			ReadjustPosition();
		}

		protected override void DisplayApp()
		{
			if (appFrame != null)
			{
				appFrame.gameObject.SetActive(value: true);
				agPanel.UpdateButtons();
			}
		}

		protected override void HideApp()
		{
			if (appFrame != null)
			{
				appFrame.gameObject.SetActive(value: false);
			}
		}

		public void HideAppFromInput()
		{
			base.appLauncherButton.SetFalse();
		}

		private void OnVesselWasModified(Vessel v)
		{
			base.appLauncherButton.VisibleInScenes = GetAppScenes();
			ApplicationLauncher.Instance.DetermineVisibility(base.appLauncherButton);
		}

		private void OnVesselChanged(Vessel v)
		{
			base.appLauncherButton.VisibleInScenes = GetAppScenes();
			ApplicationLauncher.Instance.DetermineVisibility(base.appLauncherButton);
		}

		private void OnVesselDestroy(Vessel v)
		{
		}

		private void OnVesselGoOffRails(Vessel v)
		{
		}

		private void OnVesselOverrideGroupChanged(Vessel v)
		{
			if (v == FlightGlobals.ActiveVessel)
			{
				UpdateOverrideGroupText();
				agPanel.UpdateButtons();
			}
		}

		public void UpdateOverrideGroupText()
		{
			Vessel activeVessel = FlightGlobals.ActiveVessel;
			if (!(activeVessel == null) && !(base.appLauncherButton == null))
			{
				if (overrideGroupText == null)
				{
					overrideGroupText = Object.Instantiate(overrideGroupTextPrefab);
					overrideGroupText.transform.SetParent(base.appLauncherButton.gameObject.GetChild("Image").transform, worldPositionStays: false);
				}
				int groupOverride = activeVessel.GroupOverride;
				if (groupOverride == 0)
				{
					overrideGroupText.text = "";
				}
				else
				{
					overrideGroupText.text = groupOverride.ToString();
				}
			}
		}

		public void SelectNext()
		{
			Vessel activeVessel = FlightGlobals.ActiveVessel;
			if (!(activeVessel == null))
			{
				int num = activeVessel.GroupOverride + 1;
				if (num > Vessel.NumOverrideGroups)
				{
					num = 0;
				}
				activeVessel.SetGroupOverride(num);
			}
		}

		public void SelectPrev()
		{
			Vessel activeVessel = FlightGlobals.ActiveVessel;
			if (!(activeVessel == null))
			{
				int num = activeVessel.GroupOverride - 1;
				if (num < 0)
				{
					num = Vessel.NumOverrideGroups;
				}
				activeVessel.SetGroupOverride(num);
			}
		}

		internal void HideUnpinApp()
		{
			if (!base.pinned)
			{
				HideApp();
				MouseInput_PointerExit(null);
			}
		}

		private void ReadjustPosition()
		{
			appFrameRectTransform.anchoredPosition = new Vector2(appFrameRectTransform.anchoredPosition.x - 3f, appFrameRectTransform.anchoredPosition.y);
		}
	}
}

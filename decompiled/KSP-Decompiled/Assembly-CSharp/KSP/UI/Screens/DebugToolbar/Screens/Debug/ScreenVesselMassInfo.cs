using System.Collections.Generic;
using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.SceneManagement;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar.Screens.Debug
{
	public class ScreenVesselMassInfo : MonoBehaviour
	{
		[SerializeField]
		private TextMeshProUGUI activeVesselName_text;

		[SerializeField]
		private TextMeshProUGUI vesselMass_text;

		[SerializeField]
		private Button updateStats;

		[SerializeField]
		private RectTransform contentParent;

		[SerializeField]
		private GameObject partItemPrefab;

		[SerializeField]
		private List<ScreenVesselMassPartInfo> screenObjects;

		public bool resetScreenObjects;

		[SerializeField]
		private Vessel vessel;

		[SerializeField]
		private ShipConstruct ship;

		private bool noVessel = true;

		private string cached_autoLOC_901099;

		private void Awake()
		{
			updateStats.onClick.AddListener(UpdateList);
			screenObjects = new List<ScreenVesselMassPartInfo>();
			GameEvents.onVesselChange.Add(ChangeVessel);
			GameEvents.onEditorStarted.Add(UpdateList);
			GameEvents.onEditorPodPicked.Add(onEditorPodPicked);
			GameEvents.onEditorLoad.Add(onEditorLoad);
			GameEvents.onEditorRestoreState.Add(onEditorRestoreState);
			GameEvents.onVesselWasModified.Add(ChangeVessel);
			GameEvents.onEditorShipModified.Add(ChangeShip);
			GameEvents.onEditorShipCrewModified.Add(ChangeCrew);
			SceneManager.sceneLoaded += OnSceneLoaded;
			cached_autoLOC_901099 = Localizer.Format("#autoLOC_901099");
			UpdateList();
		}

		private void Start()
		{
		}

		private void OnDestroy()
		{
			updateStats.onClick.RemoveListener(UpdateList);
			ClearScreenObjects();
			GameEvents.onVesselChange.Remove(ChangeVessel);
			GameEvents.onEditorStarted.Remove(FindVessel);
			GameEvents.onEditorPodPicked.Remove(onEditorPodPicked);
			GameEvents.onEditorLoad.Remove(onEditorLoad);
			GameEvents.onEditorRestoreState.Remove(onEditorRestoreState);
			GameEvents.onVesselWasModified.Remove(ChangeVessel);
			GameEvents.onEditorShipModified.Remove(ChangeShip);
			SceneManager.sceneLoaded -= OnSceneLoaded;
		}

		private void ClearScreenObjects()
		{
			for (int i = 0; i < screenObjects.Count; i++)
			{
				Object.Destroy(screenObjects[i].gameObject);
			}
			screenObjects.Clear();
			for (int j = 0; j < contentParent.childCount; j++)
			{
				contentParent.GetChild(j).gameObject.DestroyGameObject();
			}
			resetScreenObjects = false;
		}

		private void FindVessel()
		{
			ship = null;
			vessel = null;
			vesselMass_text.text = "";
			if (HighLogic.LoadedSceneIsEditor)
			{
				if ((bool)EditorLogic.fetch && EditorLogic.fetch.ship != null)
				{
					ship = EditorLogic.fetch.ship;
					activeVesselName_text.text = ship.shipName;
					ClearScreenObjects();
				}
			}
			else if (HighLogic.LoadedSceneIsFlight && FlightGlobals.ActiveVessel != null)
			{
				vessel = FlightGlobals.ActiveVessel;
				activeVesselName_text.text = vessel.vesselName;
				ClearScreenObjects();
			}
			noVessel = ship == null && vessel == null;
			if (noVessel)
			{
				activeVesselName_text.text = cached_autoLOC_901099;
				ClearScreenObjects();
			}
		}

		private void UpdateList()
		{
			FindVessel();
			if (!noVessel)
			{
				if (resetScreenObjects)
				{
					ClearScreenObjects();
				}
				List<Part> list = null;
				if (ship != null)
				{
					list = ship.parts;
				}
				if (vessel != null)
				{
					list = vessel.parts;
				}
				for (int i = 0; i < list.Count; i++)
				{
					Part part = list[i];
					GameObject obj = Object.Instantiate(partItemPrefab);
					obj.transform.SetParent(contentParent);
					obj.transform.localScale = Vector3.one;
					obj.GetComponent<ScreenVesselMassPartInfo>().UpdateData(part, i == 0);
				}
				if (ship != null)
				{
					vesselMass_text.text = ship.GetTotalMass().ToString("0.000");
				}
				if (vessel != null)
				{
					vesselMass_text.text = vessel.GetTotalMass().ToString("0.000");
				}
			}
		}

		private void OnSceneLoaded(Scene scene, LoadSceneMode mode)
		{
			if (mode != LoadSceneMode.Additive)
			{
				UpdateList();
			}
		}

		private void onEditorRestoreState()
		{
			UpdateList();
		}

		private void onEditorLoad(ShipConstruct ship, CraftBrowserDialog.LoadType loadType)
		{
			UpdateList();
		}

		private void onEditorPodPicked(Part part)
		{
			UpdateList();
		}

		private void ChangeVessel(Vessel vessel)
		{
			UpdateList();
		}

		private void ChangeShip(ShipConstruct ship)
		{
			UpdateList();
		}

		private void ChangeCrew(VesselCrewManifest crew)
		{
			UpdateList();
		}
	}
}

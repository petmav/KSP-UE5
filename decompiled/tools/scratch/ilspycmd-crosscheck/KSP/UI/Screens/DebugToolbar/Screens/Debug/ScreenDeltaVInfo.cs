using System.Collections.Generic;
using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.SceneManagement;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar.Screens.Debug
{
	public class ScreenDeltaVInfo : MonoBehaviour
	{
		[SerializeField]
		private TextMeshProUGUI activeVesselName_text;

		[SerializeField]
		private TextMeshProUGUI vesselStageGroups_text;

		[SerializeField]
		private TextMeshProUGUI vesselTotalDeltaV_text;

		[SerializeField]
		private TextMeshProUGUI vesselCalcType_text;

		[SerializeField]
		private TextMeshProUGUI vesselTotalBurnTime_text;

		[SerializeField]
		private Toggle showVacuum;

		[SerializeField]
		private Toggle showShowAllStages;

		[SerializeField]
		private Toggle showEngines;

		[SerializeField]
		private Toggle showParts;

		[SerializeField]
		private Toggle logVerbose;

		[SerializeField]
		private RectTransform contentParent;

		[SerializeField]
		private GameObject stageItemPrefab;

		[SerializeField]
		private GameObject engineItemPrefab;

		[SerializeField]
		private GameObject propellantItemPrefab;

		[SerializeField]
		private GameObject partsListPrefab;

		[SerializeField]
		private List<VesselDebugDeltaVInfo> screenObjects;

		public bool resetScreenObjects;

		[SerializeField]
		private VesselDeltaV vesselDeltaV;

		private string cached_autoLOC_901095;

		private string cache_autoLOC_8002204;

		private string cache_autoLOC_8002205;

		private string cache_autoLOC_8002206;

		private IComparer<VesselDebugDeltaVInfo> stageSorting = new RUIutils.FuncComparer<VesselDebugDeltaVInfo>((VesselDebugDeltaVInfo r1, VesselDebugDeltaVInfo r2) => RUIutils.SortAscDescPrimarySecondary(true, r1.stageInfo.stage.CompareTo(r2.stageInfo.stage)));

		private void Awake()
		{
			screenObjects = new List<VesselDebugDeltaVInfo>();
			GameEvents.onVesselChange.Add(ChangeVessel);
			GameEvents.onEditorStarted.Add(FindVessel);
			GameEvents.onEditorPodPicked.Add(onEditorPodPicked);
			GameEvents.onEditorLoad.Add(onEditorLoad);
			GameEvents.onEditorRestoreState.Add(onEditorRestoreState);
			SceneManager.sceneLoaded += OnSceneLoaded;
			cached_autoLOC_901095 = Localizer.Format("#autoLOC_901099");
		}

		private void Start()
		{
			CacheTypeStrings();
			logVerbose.isOn = GameSettings.LOG_DELTAV_VERBOSE;
			logVerbose.onValueChanged.AddListener(OnLogVerboseToggle);
		}

		private void Update()
		{
			FindVessel();
			if (vesselDeltaV == null || (!HighLogic.LoadedSceneIsEditor && !HighLogic.LoadedSceneIsFlight) || vesselDeltaV.SimulationRunning)
			{
				return;
			}
			if (resetScreenObjects)
			{
				ClearScreenObjects();
			}
			List<DeltaVStageInfo> operatingStageInfo = vesselDeltaV.OperatingStageInfo;
			for (int num = screenObjects.Count - 1; num >= 0; num--)
			{
				if (screenObjects[num].stageInfo == null || !operatingStageInfo.Contains(screenObjects[num].stageInfo))
				{
					screenObjects[num].Destroy();
					screenObjects.RemoveAt(num);
				}
			}
			activeVesselName_text.text = ((vesselDeltaV.ActiveMode == VesselDeltaV.Mode.Vessel) ? vesselDeltaV.Vessel.GetDisplayName() : Localizer.Format(vesselDeltaV.Ship.shipName));
			string text = "";
			CalcType calcType = ((!showVacuum.isOn) ? CalcType.ASL : CalcType.Vacuum);
			if (HighLogic.LoadedSceneIsFlight && vesselDeltaV.currentStageActivated)
			{
				double num2 = 0.0;
				for (int i = 0; i < operatingStageInfo.Count; i++)
				{
					num2 += (double)((StageManager.CurrentStage == operatingStageInfo[i].stage) ? operatingStageInfo[i].deltaVActual : (showVacuum.isOn ? operatingStageInfo[i].deltaVinVac : operatingStageInfo[i].deltaVatASL));
				}
				text = num2.ToString("N2");
				calcType = CalcType.Actual;
			}
			else
			{
				text = (showVacuum.isOn ? vesselDeltaV.TotalDeltaVVac.ToString("N2") : vesselDeltaV.TotalDeltaVASL.ToString("N2"));
			}
			vesselStageGroups_text.text = operatingStageInfo.Count.ToString();
			vesselTotalDeltaV_text.text = text;
			switch (calcType)
			{
			case CalcType.Vacuum:
				vesselCalcType_text.text = cache_autoLOC_8002204;
				break;
			case CalcType.ASL:
				vesselCalcType_text.text = cache_autoLOC_8002205;
				break;
			default:
				vesselCalcType_text.text = cache_autoLOC_8002206;
				break;
			}
			double num3 = 0.0;
			int count = operatingStageInfo.Count;
			while (true)
			{
				if (count-- > 0)
				{
					DeltaVStageInfo deltaVStageInfo = operatingStageInfo[count];
					bool num4 = HighLogic.LoadedSceneIsFlight && FlightGlobals.ActiveVessel != null && FlightGlobals.ActiveVessel.ctrlState.mainThrottle > 0f && StageManager.CurrentStage <= deltaVStageInfo.stage;
					bool flag = false;
					CalcType type = (num4 ? CalcType.Actual : ((!showVacuum.isOn) ? CalcType.ASL : CalcType.Vacuum));
					string calctypeDesc = (num4 ? cache_autoLOC_8002206 : (showVacuum.isOn ? cache_autoLOC_8002204 : cache_autoLOC_8002205));
					for (int j = 0; j < screenObjects.Count; j++)
					{
						if (screenObjects[j].stageInfo == deltaVStageInfo)
						{
							if (screenObjects[j].Update(deltaVStageInfo, type, calctypeDesc, showEngines.isOn, showParts.isOn, showShowAllStages.isOn))
							{
								flag = true;
								break;
							}
							resetScreenObjects = true;
							return;
						}
					}
					if (!flag)
					{
						VesselDebugDeltaVInfo vesselDebugDeltaVInfo = new VesselDebugDeltaVInfo(deltaVStageInfo.stage);
						if (!vesselDebugDeltaVInfo.Create(stageItemPrefab, contentParent, engineItemPrefab, propellantItemPrefab, partsListPrefab, deltaVStageInfo, type))
						{
							break;
						}
						vesselDebugDeltaVInfo.Update(deltaVStageInfo, type, calctypeDesc, showEngines.isOn, showParts.isOn, showShowAllStages.isOn);
						screenObjects.Add(vesselDebugDeltaVInfo);
					}
					num3 += deltaVStageInfo.stageBurnTime;
					continue;
				}
				vesselTotalBurnTime_text.text = KSPUtil.dateTimeFormatter.PrintTimeCompact(num3, explicitPositive: false);
				SetSiblingsUIIndex();
				return;
			}
			resetScreenObjects = true;
		}

		private void OnDestroy()
		{
			ClearScreenObjects();
			GameEvents.onVesselChange.Remove(ChangeVessel);
			GameEvents.onEditorStarted.Remove(FindVessel);
			GameEvents.onEditorPodPicked.Remove(onEditorPodPicked);
			GameEvents.onEditorLoad.Remove(onEditorLoad);
			GameEvents.onEditorRestoreState.Remove(onEditorRestoreState);
			SceneManager.sceneLoaded += OnSceneLoaded;
		}

		private void SetSiblingsUIIndex()
		{
			screenObjects.Sort(stageSorting);
			int siblingsUIIndex = 0;
			for (int i = 0; i < screenObjects.Count; i++)
			{
				siblingsUIIndex = screenObjects[i].SetSiblingsUIIndex(siblingsUIIndex);
				siblingsUIIndex++;
			}
		}

		private void ClearScreenObjects()
		{
			for (int i = 0; i < screenObjects.Count; i++)
			{
				screenObjects[i].Destroy();
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
			if (vesselDeltaV != null && vesselDeltaV.Vessel == null && vesselDeltaV.Ship == null)
			{
				UnityEngine.Debug.LogWarning("[ScreenDeltaVInfo]: Invalid VesselDeltaV reference. Resetting..");
				vesselDeltaV = null;
				ClearScreenObjects();
			}
			if (vesselDeltaV == null)
			{
				if (HighLogic.LoadedSceneIsEditor)
				{
					if ((bool)EditorLogic.fetch && EditorLogic.fetch.ship != null)
					{
						vesselDeltaV = EditorLogic.fetch.ship.vesselDeltaV;
						ClearScreenObjects();
					}
				}
				else if (HighLogic.LoadedSceneIsFlight && FlightGlobals.ActiveVessel != null)
				{
					vesselDeltaV = FlightGlobals.ActiveVessel.gameObject.GetComponent<VesselDeltaV>();
					ClearScreenObjects();
				}
			}
			if (vesselDeltaV == null)
			{
				activeVesselName_text.text = cached_autoLOC_901095;
				vesselStageGroups_text.text = Localizer.Format("#autoLOC_7003285");
				vesselTotalDeltaV_text.text = Localizer.Format("#autoLOC_7003285");
				vesselCalcType_text.text = Localizer.Format("#autoLOC_7003285");
				vesselTotalBurnTime_text.text = Localizer.Format("#autoLOC_7003285");
				ClearScreenObjects();
			}
		}

		private void OnSceneLoaded(Scene scene, LoadSceneMode mode)
		{
			if (mode != LoadSceneMode.Additive)
			{
				vesselDeltaV = null;
				FindVessel();
			}
		}

		private void onEditorRestoreState()
		{
			vesselDeltaV = null;
			FindVessel();
		}

		private void onEditorLoad(ShipConstruct ship, CraftBrowserDialog.LoadType loadType)
		{
			vesselDeltaV = null;
			FindVessel();
		}

		private void onEditorPodPicked(Part part)
		{
			vesselDeltaV = null;
			FindVessel();
		}

		private void ChangeVessel(Vessel vessel)
		{
			vesselDeltaV = null;
			FindVessel();
		}

		private void OnLogVerboseToggle(bool on)
		{
			GameSettings.LOG_DELTAV_VERBOSE = on;
			GameSettings.SaveGameSettingsOnly();
		}

		private void CacheTypeStrings()
		{
			cache_autoLOC_8002204 = Localizer.Format(CalcType.Vacuum.Description());
			cache_autoLOC_8002205 = Localizer.Format(CalcType.ASL.Description());
			cache_autoLOC_8002206 = Localizer.Format(CalcType.Actual.Description());
		}
	}
}

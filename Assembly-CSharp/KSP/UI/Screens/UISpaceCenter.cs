using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public class UISpaceCenter : MonoBehaviour
	{
		public Button quitBtn;

		public TextMeshProUGUI buildingText;

		private KSCPauseMenu pauseMenu;

		private bool _spawnedAC;

		private bool _spawnedRD;

		private bool _spawnedMC;

		private bool _spawnedVSD;

		private bool _spawnedADM;

		public static UISpaceCenter Instance { get; private set; }

		public bool SpawnedAC => _spawnedAC;

		public bool SpawnedRD => _spawnedRD;

		public bool SpawnedMC => _spawnedMC;

		public bool SpawnedVSD => _spawnedVSD;

		public bool SpawnedADM => _spawnedADM;

		private void Awake()
		{
			Instance = this;
			SetBuildingText("");
			quitBtn.onClick.AddListener(QuitToMenu);
			GameEvents.onInputLocksModified.Add(onInputLockModified);
			GameEvents.onGUIAstronautComplexSpawn.Add(OnFacilitySpawn_AC);
			GameEvents.onGUIRnDComplexSpawn.Add(OnFacilitySpawn_RD);
			GameEvents.onGUIMissionControlSpawn.Add(OnFacilitySpawn_MC);
			GameEvents.onGUILaunchScreenSpawn.Add(OnFacilitySpawn_VSD);
			GameEvents.onGUIAdministrationFacilitySpawn.Add(OnFacilitySpawn_ADM);
			GameEvents.onGUIAstronautComplexDespawn.Add(OnFacilityDespawn_AC);
			GameEvents.onGUIRnDComplexDespawn.Add(OnFacilityDespawn_RD);
			GameEvents.onGUIMissionControlDespawn.Add(OnFacilityDespawn_MC);
			GameEvents.onGUILaunchScreenDespawn.Add(OnFacilityDespawn_VSD);
			GameEvents.onGUIAdministrationFacilityDespawn.Add(OnFacilityDespawn_ADM);
		}

		private void OnFacilitySpawn_AC()
		{
			_spawnedAC = true;
			Pause();
		}

		private void OnFacilitySpawn_RD()
		{
			_spawnedRD = true;
			Pause();
		}

		private void OnFacilitySpawn_MC()
		{
			_spawnedMC = true;
			Pause();
		}

		private void OnFacilitySpawn_VSD(GameEvents.VesselSpawnInfo info)
		{
			_spawnedVSD = true;
		}

		private void OnFacilitySpawn_ADM()
		{
			_spawnedADM = true;
			Pause();
		}

		private void OnFacilityDespawn_AC()
		{
			UnPause();
			_spawnedAC = false;
		}

		private void OnFacilityDespawn_RD()
		{
			UnPause();
			_spawnedRD = false;
		}

		private void OnFacilityDespawn_MC()
		{
			UnPause();
			_spawnedMC = false;
		}

		private void OnFacilityDespawn_VSD()
		{
			_spawnedVSD = false;
		}

		private void OnFacilityDespawn_ADM()
		{
			UnPause();
			_spawnedADM = false;
		}

		private void Pause()
		{
			if (!FlightDriver.Pause)
			{
				if (TimeWarp.CurrentRateIndex != 0)
				{
					TimeWarp.SetRate(0, instant: true);
				}
				FlightDriver.SetPause(pauseState: true, postScreenMessage: false);
			}
		}

		private void UnPause()
		{
			if (FlightDriver.Pause)
			{
				FlightDriver.SetPause(pauseState: false, postScreenMessage: false);
			}
		}

		private void OnDestroy()
		{
			GameEvents.onInputLocksModified.Remove(onInputLockModified);
			GameEvents.onGUIAstronautComplexSpawn.Remove(OnFacilitySpawn_AC);
			GameEvents.onGUIRnDComplexSpawn.Remove(OnFacilitySpawn_RD);
			GameEvents.onGUIMissionControlSpawn.Remove(OnFacilitySpawn_MC);
			GameEvents.onGUILaunchScreenSpawn.Remove(OnFacilitySpawn_VSD);
			GameEvents.onGUIAdministrationFacilitySpawn.Remove(OnFacilitySpawn_ADM);
			GameEvents.onGUIAstronautComplexDespawn.Remove(OnFacilityDespawn_AC);
			GameEvents.onGUIRnDComplexDespawn.Remove(OnFacilityDespawn_RD);
			GameEvents.onGUIMissionControlDespawn.Remove(OnFacilityDespawn_MC);
			GameEvents.onGUILaunchScreenDespawn.Remove(OnFacilityDespawn_VSD);
			GameEvents.onGUIAdministrationFacilityDespawn.Remove(OnFacilityDespawn_ADM);
			if (Instance != null && Instance == this)
			{
				Instance = null;
			}
		}

		private void Update()
		{
			if (Input.GetKeyUp(KeyCode.Escape))
			{
				if (PopupDialog.CheckForOpenDialogs())
				{
					return;
				}
				if (_spawnedAC)
				{
					GameEvents.onGUIAstronautComplexDespawn.Fire();
					return;
				}
				if (_spawnedRD)
				{
					GameEvents.onGUIRnDComplexDespawn.Fire();
					return;
				}
				if (_spawnedMC)
				{
					GameEvents.onGUIMissionControlDespawn.Fire();
					return;
				}
				if (_spawnedVSD)
				{
					if (InputLockManager.IsUnlocked(ControlTypes.UI_DRAGGING) && !VesselSpawnDialog.Instance.HasSearchText)
					{
						GameEvents.onGUILaunchScreenDespawn.Fire();
					}
					return;
				}
				if (_spawnedADM)
				{
					GameEvents.onGUIAdministrationFacilityDespawn.Fire();
					return;
				}
				QuitToMenu();
			}
			if (pauseMenu == null && !InputLockManager.IsLocked(ControlTypes.KSC_UI))
			{
				if (GameSettings.QUICKSAVE.GetKeyDown())
				{
					QuitToMenu();
					pauseMenu.InitiateSave();
				}
				else if (GameSettings.QUICKLOAD.GetKeyDown())
				{
					QuitToMenu();
					pauseMenu.InitiateLoad();
				}
			}
		}

		private void OnApplicationFocus(bool focus)
		{
			if (!focus)
			{
				InputLockManager.SetControlLock("ksc_ApplicationFocus");
			}
			else
			{
				InputLockManager.RemoveControlLock("ksc_ApplicationFocus");
			}
		}

		private void onInputLockModified(GameEvents.FromToAction<ControlTypes, ControlTypes> ctrls)
		{
			if (InputLockManager.IsLocking(ControlTypes.KSC_UI, ctrls))
			{
				lockUI();
			}
			if (InputLockManager.IsUnlocking(ControlTypes.KSC_UI, ctrls))
			{
				unlockUI();
			}
		}

		private void lockUI()
		{
			quitBtn.Lock();
		}

		private void unlockUI()
		{
			quitBtn.Unlock();
		}

		private void QuitToMenu()
		{
			if (!InputLockManager.IsLocked(ControlTypes.KSC_UI) && !(pauseMenu != null))
			{
				pauseMenu = KSCPauseMenu.Create(OnPauseMenuDismiss);
				pauseMenu.BuildButtonList(pauseMenu.dialogObj);
			}
		}

		private void OnPauseMenuDismiss()
		{
			pauseMenu = null;
		}

		public void SetBuildingText(string text)
		{
			if (buildingText != null && buildingText.gameObject.activeInHierarchy)
			{
				buildingText.text = text;
			}
		}
	}
}

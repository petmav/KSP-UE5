using SentinelMission;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar.Screens.Cheats
{
	public class CheatsComets : MonoBehaviour
	{
		public Button KillButton;

		public Button genRandomButton;

		public Button genShortButton;

		public Button genMidButton;

		public Button genLongButton;

		public Button genInterstellarButton;

		public Button sentinelBackButton;

		public TextMeshProUGUI sentinelNameText;

		public Button sentinelForwardButton;

		public Button genSentinelButton;

		public Button genSentinelShortButton;

		public Button genSentinelMidButton;

		public Button genSentinelLongButton;

		public Button genSentinelInterButton;

		public Toggle collidersEnabled;

		public Toggle geysersEnabled;

		public Toggle dustsEnabled;

		public Toggle comaEnabled;

		public Toggle ionTailEnabled;

		public Toggle dustTailEnabled;

		public Slider geyserEmitterMultiplier;

		public TextMeshProUGUI geyserMultiplierText;

		public Slider snowEmitterMultiplier;

		public TextMeshProUGUI snowMultiplierText;

		public Slider ionEmitterMultiplier;

		public TextMeshProUGUI ionMultiplierText;

		public Slider dustEmitterMultiplier;

		public TextMeshProUGUI dustMultiplierText;

		public Slider dustSizeMultiplier;

		public TextMeshProUGUI dustSizeMultiplierText;

		public Button rePrimeParticles;

		private int selectedSentinel;

		private SentinelScenario sentinelScenario;

		private float gametime;

		public Vessel SelectedSentinel
		{
			get
			{
				if (sentinelScenario != null && sentinelScenario.deployedSentinels.Count > selectedSentinel)
				{
					return sentinelScenario.deployedSentinels[selectedSentinel];
				}
				return null;
			}
		}

		private void Awake()
		{
			KillButton.onClick.AddListener(OnKillClick);
			genRandomButton.onClick.AddListener(OnRandomClick);
			genShortButton.onClick.AddListener(OnShortClick);
			genMidButton.onClick.AddListener(OnMidClick);
			genLongButton.onClick.AddListener(OnLongClick);
			genInterstellarButton.onClick.AddListener(OnInterstellarClick);
			sentinelBackButton.onClick.AddListener(OnVesselBackClick);
			sentinelForwardButton.onClick.AddListener(OnVesselForwardClick);
			genSentinelButton.onClick.AddListener(OnSentinelClick);
			genSentinelShortButton.onClick.AddListener(OnSentinelShortClick);
			genSentinelMidButton.onClick.AddListener(OnSentinelMidClick);
			genSentinelLongButton.onClick.AddListener(OnSentinelLongClick);
			genSentinelInterButton.onClick.AddListener(OnSentinelInterstellarClick);
			collidersEnabled.onValueChanged.AddListener(OnCollidersClick);
			geysersEnabled.onValueChanged.AddListener(OnGeysersClick);
			dustsEnabled.onValueChanged.AddListener(OnDustClick);
			comaEnabled.onValueChanged.AddListener(OnComaClick);
			ionTailEnabled.onValueChanged.AddListener(OnIonTailClick);
			dustTailEnabled.onValueChanged.AddListener(OnDustTailClick);
			geyserEmitterMultiplier.onValueChanged.AddListener(OnGeyserMultiplierChange);
			snowEmitterMultiplier.onValueChanged.AddListener(OnSnowMultiplierChange);
			ionEmitterMultiplier.onValueChanged.AddListener(OnIonMultiplierChange);
			dustEmitterMultiplier.onValueChanged.AddListener(OnDustMultiplierChange);
			dustSizeMultiplier.onValueChanged.AddListener(OnDustSizeMultiplierChange);
			rePrimeParticles.onClick.AddListener(OnReprimeParticles);
		}

		private void Start()
		{
			sentinelScenario = SentinelScenario.Instance;
			if (sentinelScenario != null)
			{
				selectedSentinel = 0;
			}
			UpdateSentinelOptions();
			gametime = Time.realtimeSinceStartup;
		}

		private void Update()
		{
			if (sentinelScenario != null)
			{
				gametime += Time.unscaledDeltaTime;
				if (gametime > 3f)
				{
					gametime = 3f;
					UpdateSentinelOptions();
				}
			}
		}

		private void UpdateSentinelOptions()
		{
			sentinelBackButton.gameObject.SetActive(sentinelScenario != null);
			sentinelNameText.gameObject.SetActive(sentinelScenario != null);
			sentinelForwardButton.gameObject.SetActive(sentinelScenario != null);
			genSentinelButton.gameObject.SetActive(sentinelScenario != null);
			SetSelectedSentinelText();
		}

		private void OnKillClick()
		{
			if (FlightGlobals.fetch == null)
			{
				return;
			}
			int count = FlightGlobals.Vessels.Count;
			while (count-- > 0)
			{
				if (FlightGlobals.Vessels[count].Comet != null)
				{
					FlightGlobals.Vessels[count].Die();
				}
			}
		}

		private void OnRandomClick()
		{
			if (!(ScenarioDiscoverableObjects.Instance == null))
			{
				ScenarioDiscoverableObjects.Instance.SpawnComet();
			}
		}

		private void OnShortClick()
		{
			if (!(ScenarioDiscoverableObjects.Instance == null))
			{
				ScenarioDiscoverableObjects.Instance.SpawnComet("short");
			}
		}

		private void OnMidClick()
		{
			if (!(ScenarioDiscoverableObjects.Instance == null))
			{
				ScenarioDiscoverableObjects.Instance.SpawnComet("intermediate");
			}
		}

		private void OnLongClick()
		{
			if (!(ScenarioDiscoverableObjects.Instance == null))
			{
				ScenarioDiscoverableObjects.Instance.SpawnComet("long");
			}
		}

		private void OnInterstellarClick()
		{
			if (!(ScenarioDiscoverableObjects.Instance == null))
			{
				ScenarioDiscoverableObjects.Instance.SpawnComet("interstellar");
			}
		}

		private void OnSentinelClick()
		{
			if (!(sentinelScenario == null))
			{
				sentinelScenario.SpawnComet(SelectedSentinel);
			}
		}

		private void OnSentinelShortClick()
		{
			if (!(sentinelScenario == null))
			{
				sentinelScenario.SpawnComet(SelectedSentinel, "short");
			}
		}

		private void OnSentinelMidClick()
		{
			if (!(sentinelScenario == null))
			{
				sentinelScenario.SpawnComet(SelectedSentinel, "intermediate");
			}
		}

		private void OnSentinelLongClick()
		{
			if (!(sentinelScenario == null))
			{
				sentinelScenario.SpawnComet(SelectedSentinel, "long");
			}
		}

		private void OnSentinelInterstellarClick()
		{
			if (!(sentinelScenario == null))
			{
				sentinelScenario.SpawnComet(SelectedSentinel, "interstellar");
			}
		}

		private void OnVesselBackClick()
		{
			if (!(sentinelScenario == null))
			{
				selectedSentinel--;
				if (selectedSentinel < 0)
				{
					selectedSentinel = sentinelScenario.deployedSentinels.Count - 1;
				}
				SetSelectedSentinelText();
			}
		}

		private void OnVesselForwardClick()
		{
			if (!(sentinelScenario == null))
			{
				selectedSentinel++;
				if (selectedSentinel >= sentinelScenario.deployedSentinels.Count)
				{
					selectedSentinel = 0;
				}
				SetSelectedSentinelText();
			}
		}

		private void SetSelectedSentinelText()
		{
			Vessel vessel = SelectedSentinel;
			sentinelNameText.text = ((vessel != null) ? vessel.GetDisplayName() : string.Empty);
		}

		private void OnCollidersClick(bool value)
		{
			if (CometManager.Instance != null)
			{
				CometManager.Instance.SetCollider(value);
			}
		}

		private void OnGeysersClick(bool value)
		{
			if (CometManager.Instance != null)
			{
				CometManager.Instance.SetGeyser(value, geyserEmitterMultiplier.value / 100f);
			}
		}

		private void OnDustClick(bool value)
		{
			if (CometManager.Instance != null)
			{
				CometManager.Instance.SetDust(value, snowEmitterMultiplier.value / 100f);
			}
		}

		private void OnComaClick(bool value)
		{
			if (CometManager.Instance != null)
			{
				CometManager.Instance.SetComa(value);
			}
		}

		private void OnIonTailClick(bool value)
		{
			if (CometManager.Instance != null)
			{
				CometManager.Instance.SetIonTail(value, ionEmitterMultiplier.value / 100f);
			}
		}

		private void OnDustTailClick(bool value)
		{
			if (CometManager.Instance != null)
			{
				CometManager.Instance.SetDustTail(value, dustEmitterMultiplier.value / 100f, dustSizeMultiplier.value / 100f);
			}
		}

		private void OnGeyserMultiplierChange(float value)
		{
			if (CometManager.Instance != null)
			{
				geyserMultiplierText.text = $"{value:0}%";
				CometManager.Instance.SetGeyser(geysersEnabled.isOn, value / 100f);
			}
		}

		private void OnSnowMultiplierChange(float value)
		{
			if (CometManager.Instance != null)
			{
				snowMultiplierText.text = $"{value:0}%";
				CometManager.Instance.SetDust(dustsEnabled.isOn, value / 100f);
			}
		}

		private void OnDustMultiplierChange(float value)
		{
			if (CometManager.Instance != null)
			{
				dustMultiplierText.text = $"{value:0}%";
				CometManager.Instance.SetDustTail(dustTailEnabled.isOn, value / 100f, dustSizeMultiplier.value / 100f);
			}
		}

		private void OnIonMultiplierChange(float value)
		{
			if (CometManager.Instance != null)
			{
				ionMultiplierText.text = $"{value:0}%";
				CometManager.Instance.SetIonTail(ionTailEnabled.isOn, value / 100f);
			}
		}

		private void OnDustSizeMultiplierChange(float value)
		{
			if (CometManager.Instance != null)
			{
				dustSizeMultiplierText.text = $"{value:0}%";
				CometManager.Instance.SetDustTail(dustTailEnabled.isOn, dustEmitterMultiplier.value / 100f, value / 100f);
			}
		}

		private void OnReprimeParticles()
		{
			if (CometManager.Instance != null)
			{
				CometManager.Instance.ReprimeParticles();
			}
		}
	}
}

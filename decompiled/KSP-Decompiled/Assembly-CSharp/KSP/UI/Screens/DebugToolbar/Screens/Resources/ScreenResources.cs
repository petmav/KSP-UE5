using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar.Screens.Resources
{
	public class ScreenResources : MonoBehaviour
	{
		public Toggle heatGenerationToggle;

		public Toggle debugInfoToggle;

		private void Start()
		{
			if (ResourceSetup.Instance != null)
			{
				heatGenerationToggle.isOn = ResourceSetup.Instance.ResConfig.HeatEnabled;
				debugInfoToggle.isOn = ResourceSetup.Instance.ResConfig.ShowDebugOptions;
			}
			AddListeners();
		}

		private void AddListeners()
		{
			heatGenerationToggle.onValueChanged.AddListener(OnHeatGenerationToggle);
			debugInfoToggle.onValueChanged.AddListener(OnDebugInfoToggle);
		}

		private void OnHeatGenerationToggle(bool on)
		{
			if (ResourceSetup.Instance != null)
			{
				ResourceSetup.Instance.ResConfig.HeatEnabled = on;
			}
		}

		private void OnDebugInfoToggle(bool on)
		{
			if (ResourceSetup.Instance != null)
			{
				ResourceSetup.Instance.ResConfig.ShowDebugOptions = on;
			}
		}
	}
}

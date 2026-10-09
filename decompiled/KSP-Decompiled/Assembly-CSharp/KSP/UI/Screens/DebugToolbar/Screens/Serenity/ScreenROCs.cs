using System;
using System.Text;
using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar.Screens.Serenity
{
	public class ScreenROCs : MonoBehaviour
	{
		public Toggle rocFinder;

		public Toggle rocScanPoints;

		public Toggle rocStatsEnabled;

		public TextMeshProUGUI statsField;

		private void Start()
		{
			if (ROCManager.Instance != null)
			{
				rocFinder.isOn = ROCManager.Instance.debugROCFinder;
				rocScanPoints.isOn = ROCManager.Instance.debugROCScanPoints;
				rocStatsEnabled.isOn = ROCManager.Instance.debugROCStats;
				ROCManager instance = ROCManager.Instance;
				instance.OnStatsChanged = (Callback)Delegate.Combine(instance.OnStatsChanged, new Callback(OnStatsChanged));
			}
			else
			{
				rocFinder.isOn = false;
				rocScanPoints.isOn = false;
				rocStatsEnabled.isOn = false;
			}
			statsField.text = "";
			AddListeners();
		}

		private void OnDestroy()
		{
			ROCManager instance = ROCManager.Instance;
			instance.OnStatsChanged = (Callback)Delegate.Remove(instance.OnStatsChanged, new Callback(OnStatsChanged));
		}

		private void AddListeners()
		{
			rocFinder.onValueChanged.AddListener(OnROCFinderToggle);
			rocScanPoints.onValueChanged.AddListener(OnROCScanPointsToggle);
			rocStatsEnabled.onValueChanged.AddListener(OnROCStatsEnabledToggle);
		}

		private void OnROCFinderToggle(bool on)
		{
			if (ROCManager.Instance != null)
			{
				ROCManager.Instance.debugROCFinder = on;
				GameEvents.OnDebugROCFinderToggled.Fire();
			}
		}

		private void OnROCScanPointsToggle(bool on)
		{
			if (ROCManager.Instance != null)
			{
				ROCManager.Instance.debugROCScanPoints = on;
				GameEvents.OnDebugROCFinderToggled.Fire();
			}
		}

		private void OnROCStatsEnabledToggle(bool on)
		{
			if (ROCManager.Instance != null)
			{
				ROCManager.Instance.debugROCStats = on;
				GameSettings.COLLECT_ROC_STATS = on;
				GameSettings.SaveGameSettingsOnly();
			}
		}

		private void OnStatsChanged()
		{
			if (ROCManager.Instance != null)
			{
				StringBuilder stringBuilder = StringBuilderCache.Acquire();
				stringBuilder.Append(Localizer.Format("#autoLOC_8004453")).Append("\n\n");
				for (int i = 0; i < ROCManager.Instance.rocStats.ValuesList.Count; i++)
				{
					ROCManager.ROCStats rOCStats = ROCManager.Instance.rocStats.ValuesList[i];
					stringBuilder.Append(Localizer.Format("#autoLOC_8004456", rOCStats.rocType, rOCStats.activeQuads, rOCStats.activeQuadArea, rOCStats.activeRocCount, rOCStats.rocCoverage.ToString("F2"), rOCStats.rocTypeFrequency.ToString("F2")));
				}
				statsField.text = stringBuilder.ToStringAndRelease();
			}
		}
	}
}

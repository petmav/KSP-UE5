using KSP.Localization;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar.Screens.Resources
{
	public class Debugging : MonoBehaviour
	{
		public Toggle printErrorsToScreenToggle;

		public Toggle printExceptionsToScreenToggle;

		public Toggle logInstantFlushToggle;

		public Toggle logMissingLocalizationKeys;

		public Toggle showLocalizationKeys;

		public Toggle showVesselNamingParts;

		public Toggle uiMasterPixelPerfect;

		private void Start()
		{
			printErrorsToScreenToggle.isOn = GameSettings.LOG_ERRORS_TO_SCREEN;
			printExceptionsToScreenToggle.isOn = GameSettings.LOG_EXCEPTIONS_TO_SCREEN;
			logInstantFlushToggle.isOn = GameSettings.LOG_INSTANT_FLUSH;
			logMissingLocalizationKeys.isOn = GameSettings.LOG_MISSING_KEYS_TO_FILE;
			showLocalizationKeys.isOn = GameSettings.SHOW_TRANSLATION_KEYS_ON_SCREEN;
			showVesselNamingParts.isOn = false;
			uiMasterPixelPerfect.isOn = GameSettings.UI_MAINCANVAS_PIXEL_PERFECT;
			AddListeners();
		}

		private void AddListeners()
		{
			printErrorsToScreenToggle.onValueChanged.AddListener(OnErrorToggle);
			printExceptionsToScreenToggle.onValueChanged.AddListener(OnExceptionToggle);
			logInstantFlushToggle.onValueChanged.AddListener(OnInstantFlushToggle);
			logMissingLocalizationKeys.onValueChanged.AddListener(OnMissingKeysToggle);
			showLocalizationKeys.onValueChanged.AddListener(OnShowLocalizationKeysToggle);
			showVesselNamingParts.onValueChanged.AddListener(OnShowVesselNamingPartsToggle);
			uiMasterPixelPerfect.onValueChanged.AddListener(OnUIMasterPixelPerfectToggle);
		}

		private void OnMissingKeysToggle(bool on)
		{
			GameSettings.LOG_MISSING_KEYS_TO_FILE = on;
			GameSettings.SaveSettings();
			Localizer.debugWriteMissingKeysToLog = on;
		}

		private void OnShowLocalizationKeysToggle(bool on)
		{
			GameSettings.SHOW_TRANSLATION_KEYS_ON_SCREEN = on;
			GameSettings.SaveSettings();
			Localizer.ShowKeysOnScreen = on;
		}

		private void OnShowVesselNamingPartsToggle(bool on)
		{
		}

		private void OnUIMasterPixelPerfectToggle(bool on)
		{
			GameSettings.UI_MAINCANVAS_PIXEL_PERFECT = on;
			GameSettings.SaveSettings();
			UIMasterController.Instance.SetUIMainCanvasPixelPerfect(GameSettings.UI_MAINCANVAS_PIXEL_PERFECT);
		}

		private void OnErrorToggle(bool on)
		{
			bool num = GameSettings.LOG_ERRORS_TO_SCREEN || GameSettings.LOG_EXCEPTIONS_TO_SCREEN;
			GameSettings.LOG_ERRORS_TO_SCREEN = on;
			bool screenLogging;
			if (num != (screenLogging = GameSettings.LOG_ERRORS_TO_SCREEN || GameSettings.LOG_EXCEPTIONS_TO_SCREEN) && KSPLog.Instance != null)
			{
				KSPLog.Instance.SetScreenLogging(screenLogging);
			}
			GameSettings.SaveSettings();
		}

		private void OnExceptionToggle(bool on)
		{
			bool num = GameSettings.LOG_ERRORS_TO_SCREEN || GameSettings.LOG_EXCEPTIONS_TO_SCREEN;
			GameSettings.LOG_EXCEPTIONS_TO_SCREEN = on;
			bool screenLogging;
			if (num != (screenLogging = GameSettings.LOG_ERRORS_TO_SCREEN || GameSettings.LOG_EXCEPTIONS_TO_SCREEN) && KSPLog.Instance != null)
			{
				KSPLog.Instance.SetScreenLogging(screenLogging);
			}
			GameSettings.SaveSettings();
		}

		private void OnInstantFlushToggle(bool on)
		{
			GameSettings.LOG_INSTANT_FLUSH = on;
			GameSettings.SaveSettings();
		}
	}
}

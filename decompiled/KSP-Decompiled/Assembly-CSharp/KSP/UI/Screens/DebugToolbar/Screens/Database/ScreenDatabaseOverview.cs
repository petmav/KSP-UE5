using System.Collections;
using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar.Screens.Database
{
	public class ScreenDatabaseOverview : MonoBehaviour
	{
		public Button reloadButton;

		public Button reloadTextsButton;

		public TextMeshProUGUI partCount;

		public TextMeshProUGUI propCount;

		public TextMeshProUGUI internalCount;

		public TextMeshProUGUI resourceCount;

		public TextMeshProUGUI localizedStringCount;

		public TextMeshProUGUI assemblyCount;

		public TextMeshProUGUI modelCount;

		public TextMeshProUGUI textureCount;

		public TextMeshProUGUI audioCount;

		private bool isRecompiling;

		private void Start()
		{
			ResetCounts();
			CheckInteractable(HighLogic.LoadedScene);
			reloadButton.onClick.AddListener(OnReloadClicked);
			reloadTextsButton.onClick.AddListener(OnReloadTextsClicked);
			GameEvents.onLevelWasLoaded.Add(CheckInteractable);
		}

		private void OnDestroy()
		{
			GameEvents.onLevelWasLoaded.Remove(CheckInteractable);
		}

		private void OnReloadClicked()
		{
			GameDatabase.Instance.StartCoroutine(RecompileAssets());
		}

		private void OnReloadTextsClicked()
		{
			bool num = Localizer.SwitchToLanguage(Localizer.GetLanguageIdFromFile());
			string message = (num ? "Language dictionary reloaded successfully." : "Unable to reload the language dictionary. Check the log for errors.");
			if (num)
			{
				UnityEngine.Debug.Log(message);
			}
			else
			{
				UnityEngine.Debug.LogError(message);
			}
			ScreenMessages.PostScreenMessage(message, 5f, ScreenMessageStyle.UPPER_CENTER);
		}

		private void CheckInteractable(GameScenes scene)
		{
			reloadButton.interactable = scene != GameScenes.FLIGHT && scene != GameScenes.EDITOR && !isRecompiling;
		}

		private void ResetCounts()
		{
			if (GameDatabase.Instance == null)
			{
				FlushCounts("N/A");
				return;
			}
			assemblyCount.text = AssemblyLoader.loadedAssemblies.Count.ToString();
			modelCount.text = GameDatabase.Instance.databaseModel.Count.ToString();
			textureCount.text = GameDatabase.Instance.databaseTexture.Count.ToString();
			audioCount.text = GameDatabase.Instance.databaseAudio.Count.ToString();
			partCount.text = GameDatabase.Instance.GetConfigs("PART").Length.ToString();
			propCount.text = GameDatabase.Instance.GetConfigs("PROP").Length.ToString();
			internalCount.text = GameDatabase.Instance.GetConfigs("INTERNAL").Length.ToString();
			resourceCount.text = GameDatabase.Instance.GetConfigs("RESOURCE").Length.ToString();
			localizedStringCount.text = Localizer.TagsLength.ToString();
		}

		private void FlushCounts(string s)
		{
			partCount.text = s;
			propCount.text = s;
			internalCount.text = s;
			resourceCount.text = s;
			localizedStringCount.text = s;
			assemblyCount.text = s;
			modelCount.text = s;
			textureCount.text = s;
			audioCount.text = s;
		}

		private IEnumerator RecompileAssets()
		{
			if (isRecompiling)
			{
				UnityEngine.Debug.LogError("Database is already recompiling!");
				yield break;
			}
			FlushCounts(Localizer.Format("#autoLOC_7001100"));
			reloadButton.interactable = false;
			isRecompiling = true;
			if ((bool)GameDatabase.Instance)
			{
				GameDatabase.Instance.Recompile = true;
				GameDatabase.Instance.StartLoad();
				while (!GameDatabase.Instance.IsReady())
				{
					yield return null;
				}
			}
			else
			{
				UnityEngine.Debug.LogError("GameDatabase is not available to reload right now.");
			}
			if ((bool)PartLoader.Instance)
			{
				PartLoader.Instance.StartLoad();
				while (!PartLoader.Instance.IsReady())
				{
					yield return null;
				}
			}
			else
			{
				UnityEngine.Debug.LogError("PartLoader is not available to reload right now.");
			}
			if (GameDatabase.Instance != null)
			{
				GameDatabase.Instance.Recompile = false;
			}
			isRecompiling = false;
			CheckInteractable(HighLogic.LoadedScene);
			ResetCounts();
		}
	}
}

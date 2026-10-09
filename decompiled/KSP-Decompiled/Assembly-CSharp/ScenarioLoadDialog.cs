using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.Networking;
using UnityEngine.UI;

public class ScenarioLoadDialog : MonoBehaviour
{
	public delegate void FinishedCallback(ScenarioSaveInfo save);

	public class ScenarioSaveInfo
	{
		public class GameStateInfo
		{
			public Game game;

			public string fullFilePath;

			public int vesselCount;

			public int UT = -1;

			public int funds;

			public int science;

			public int reputationPercent;

			public int ongoingContracts;
		}

		public string name;

		public Texture2D banner;

		public GameStateInfo Initial;

		public GameStateInfo Current;

		public string savePath;

		public string resumeFolderFullPath;

		public GameStateInfo ToLoad
		{
			get
			{
				if (!CanRestart())
				{
					return Initial;
				}
				return Current;
			}
		}

		public event Callback<Texture2D> OnBannerLoaded = delegate
		{
		};

		public void SetBanner(Texture2D texture)
		{
			banner = texture;
			this.OnBannerLoaded(banner);
		}

		public bool CanRestart()
		{
			if (Initial.game.IsResumable())
			{
				return Initial.game != Current.game;
			}
			return false;
		}

		public string LoadPath()
		{
			if (CanRestart())
			{
				return Current.fullFilePath;
			}
			return Initial.fullFilePath;
		}
	}

	public enum ScenarioSet
	{
		[Description("#autoLOC_7001034")]
		Scenarios,
		[Description("#autoLOC_7001035")]
		Training
	}

	public Texture2D scenarioIcon;

	public Texture2D tutorialIcon;

	public Material iconMaterial;

	public Color scnIconColor;

	public Color tutIconColor;

	public List<string> blacklistScenarios = new List<string>();

	[SerializeField]
	private Button btnLoad;

	[SerializeField]
	private TextMeshProUGUI btnLoadCaption;

	[SerializeField]
	private Button btnCancel;

	[SerializeField]
	private Button btnRestart;

	[SerializeField]
	private Button btnScnLink;

	[SerializeField]
	private TextMeshProUGUI btnScnLinkCaption;

	[SerializeField]
	private TextMeshProUGUI textScnDescription;

	[SerializeField]
	private TextMeshProUGUI textScnTitle;

	[SerializeField]
	private RawImage imgScnBanner;

	[SerializeField]
	private RectTransform scrollListContent;

	[SerializeField]
	private ToggleGroup listGroup;

	[SerializeField]
	private TextMeshProUGUI header;

	private List<DialogGUIToggleButton> items = new List<DialogGUIToggleButton>();

	private FinishedCallback OnFinishedCallback = delegate
	{
	};

	private UISkinDef skin;

	private string directory;

	private bool confirmGameRestart;

	private ScenarioSet scnSet;

	private List<ScenarioSaveInfo> saves;

	private ScenarioSaveInfo selectedGame;

	private MenuNavigation menuNav;

	private static Dictionary<string, Texture2D> bannerDictionary = new Dictionary<string, Texture2D>(20);

	public static ScenarioLoadDialog Create(FinishedCallback onDismiss, ScenarioSet scnSet, UISkinDef skin)
	{
		GameObject obj = UnityEngine.Object.Instantiate(AssetBase.GetPrefab("ScenarioLoadDialog"));
		obj.name = "LoadScenario";
		obj.transform.SetParent(DialogCanvasUtil.DialogCanvasRect, worldPositionStays: false);
		obj.transform.localPosition = Vector3.zero;
		ScenarioLoadDialog component = obj.GetComponent<ScenarioLoadDialog>();
		component.OnFinishedCallback = onDismiss;
		component.skin = skin;
		component.scnSet = scnSet;
		component.directory = scnSet.ToString().ToLower();
		component.GatherSaveFiles();
		component.BuildList();
		return component;
	}

	protected void Start()
	{
		header.text = Localizer.Format(scnSet.displayDescription());
		btnRestart.interactable = false;
		btnLoad.interactable = false;
		btnRestart.onClick.AddListener(OnButtonRestart);
		btnCancel.onClick.AddListener(OnBtnCancel);
		btnLoad.onClick.AddListener(OnButtonLoad);
		btnScnLink.onClick.AddListener(OnBtnScnLink);
		btnScnLink.gameObject.SetActive(value: false);
		imgScnBanner.gameObject.SetActive(value: false);
		textScnTitle.text = Localizer.Format("#autoLOC_464934");
		textScnDescription.text = Localizer.Format("#autoLOC_464935");
		MenuNavigation.SpawnMenuNavigation(base.gameObject, Navigation.Mode.Automatic, SliderFocusType.Scrollbar);
		SetExplicitNavigation();
	}

	private void SetExplicitNavigation()
	{
		menuNav = GetComponent<MenuNavigation>();
		Selectable selectable = null;
		Selectable selectable2 = null;
		Selectable selectable3 = null;
		Selectable selectOnUp = menuNav.selectableItems[menuNav.selectableItems.Count - 4];
		for (int i = 0; i < menuNav.selectableItems.Count; i++)
		{
			if (menuNav.selectableItems[i].gameObject.name.Contains("Restart"))
			{
				selectable = menuNav.selectableItems[i];
			}
			else if (menuNav.selectableItems[i].gameObject.name.Contains("Cancel"))
			{
				selectable2 = menuNav.selectableItems[i];
			}
			else if (menuNav.selectableItems[i].gameObject.name.Contains("Load"))
			{
				selectable3 = menuNav.selectableItems[i];
			}
		}
		if (selectable != null && selectable3 != null && selectable2 != null)
		{
			Navigation navigation = default(Navigation);
			navigation.mode = Navigation.Mode.Explicit;
			navigation.selectOnRight = selectable2;
			navigation.selectOnUp = selectOnUp;
			selectable.navigation = navigation;
			selectable.gameObject.AddComponent(typeof(UINavExplicit));
			Navigation navigation2 = default(Navigation);
			navigation2.mode = Navigation.Mode.Explicit;
			navigation2.selectOnRight = selectable3;
			navigation2.selectOnUp = selectOnUp;
			navigation2.selectOnLeft = selectable;
			selectable2.navigation = navigation2;
			selectable2.gameObject.AddComponent(typeof(UINavExplicit));
			Navigation navigation3 = default(Navigation);
			navigation3.mode = Navigation.Mode.Explicit;
			navigation3.selectOnUp = selectOnUp;
			navigation3.selectOnLeft = selectable2;
			selectable3.navigation = navigation3;
			selectable3.gameObject.AddComponent(typeof(UINavExplicit));
		}
	}

	public void Terminate()
	{
		UnityEngine.Object.Destroy(base.gameObject);
	}

	protected void OnDestroy()
	{
		ClearListItems();
		foreach (KeyValuePair<string, Texture2D> item in bannerDictionary)
		{
			UnityEngine.Object.Destroy(item.Value);
		}
		bannerDictionary.Clear();
		saves.Clear();
		FlightGlobals.ClearpersistentIdDictionaries();
	}

	public void UpdateBlacklist()
	{
		if (blacklistScenarios == null)
		{
			blacklistScenarios = new List<string>();
		}
		else
		{
			blacklistScenarios.Clear();
		}
		ConfigNode[] configNodes = GameDatabase.Instance.GetConfigNodes("SCENARIO_BLACKLIST");
		int num = configNodes.Length;
		while (num-- > 0)
		{
			ConfigNode configNode = configNodes[num];
			int count = configNode.values.Count;
			while (count-- > 0)
			{
				blacklistScenarios.Add(configNode.values[count].value);
			}
		}
	}

	private void GatherSaveFiles()
	{
		UpdateBlacklist();
		string text = KSPUtil.ApplicationRootPath + "saves/" + directory;
		saves = new List<ScenarioSaveInfo>();
		if (Directory.Exists(text))
		{
			string[] files = Directory.GetFiles(text, "*.sfs", SearchOption.TopDirectoryOnly);
			int num = files.Length;
			for (int i = 0; i < num; i++)
			{
				if (!blacklistScenarios.Contains(Path.GetFileName(files[i])))
				{
					ScenarioSaveInfo scenarioSaveInfo = GatherScenarioInfo(files[i], text);
					if (scenarioSaveInfo != null)
					{
						saves.Add(scenarioSaveInfo);
					}
				}
			}
		}
		selectedGame = null;
	}

	protected ScenarioSaveInfo GatherScenarioInfo(string file, string pathRoot)
	{
		ScenarioSaveInfo save = new ScenarioSaveInfo();
		save.name = Path.GetFileNameWithoutExtension(file);
		save.savePath = directory + "/" + save.name;
		save.Initial = new ScenarioSaveInfo.GameStateInfo();
		save.Current = new ScenarioSaveInfo.GameStateInfo();
		save.Initial.fullFilePath = file;
		Game game = GamePersistence.LoadGame(save.name, directory, nullIfIncompatible: false, suppressIncompatibleMessage: false);
		save.Initial.game = game;
		save.resumeFolderFullPath = pathRoot + "/" + save.name;
		save.Current.fullFilePath = save.resumeFolderFullPath + "/persistent.sfs";
		if (Directory.Exists(save.resumeFolderFullPath) && File.Exists(save.resumeFolderFullPath + "/persistent.sfs"))
		{
			game = GamePersistence.LoadGame("persistent", save.savePath, nullIfIncompatible: false, suppressIncompatibleMessage: false);
		}
		save.Current.game = game;
		StartCoroutine(LoadBannerTexture(save, pathRoot));
		save.OnBannerLoaded += delegate(Texture2D tx)
		{
			if (save == selectedGame)
			{
				imgScnBanner.texture = tx;
				imgScnBanner.gameObject.SetActive(value: true);
			}
		};
		return save;
	}

	private IEnumerator LoadBannerTexture(ScenarioSaveInfo scn, string folderPath)
	{
		string filePathToLoad = folderPath + "/banners/" + scn.name + ".png";
		if (!File.Exists(filePathToLoad))
		{
			filePathToLoad = folderPath + "/banners/default.png";
		}
		Texture2D value = null;
		if (!bannerDictionary.TryGetValue(filePathToLoad, out value))
		{
			UnityEngine.Debug.Log("Loading banner texture from " + filePathToLoad);
			using (UnityWebRequest loader = UnityWebRequestTexture.GetTexture(KSPUtil.ApplicationFileProtocol + filePathToLoad))
			{
				yield return loader.SendWebRequest();
				while (!loader.isDone)
				{
					yield return null;
				}
				if (loader.isNetworkError || loader.isHttpError || !string.IsNullOrEmpty(loader.error))
				{
					UnityEngine.Debug.LogError("LoadBannerTexture - WWW error in " + KSPUtil.ApplicationFileProtocol + filePathToLoad + " : " + loader.error);
					yield break;
				}
				Texture2D texture = ((DownloadHandlerTexture)loader.downloadHandler).texture;
				value = new Texture2D(texture.width, texture.height, TextureFormat.ARGB32, mipChain: false);
				value.SetPixels32(texture.GetPixels32());
				value.Apply();
				bannerDictionary.Add(filePathToLoad, value);
			}
		}
		scn.SetBanner(value);
	}

	private void BuildList()
	{
		SetHidden(hide: false);
		ClearListItems();
		Texture texture = null;
		ScenarioSet scenarioSet = scnSet;
		texture = ((scenarioSet == ScenarioSet.Scenarios || scenarioSet != ScenarioSet.Training) ? scenarioIcon : tutorialIcon);
		int count = saves.Count;
		for (int i = 0; i < count; i++)
		{
			ScenarioSaveInfo save = saves[i];
			DialogGUIToggleButton dialogGUIToggleButton = new DialogGUIToggleButton(set: false, string.Empty, delegate(bool b)
			{
				if (b)
				{
					selectedGame = save;
					if ((Mouse.Left.GetDoubleClick(isDelegate: true) || menuNav.SumbmitOnSelectedToggle()) && !confirmGameRestart)
					{
						ConfirmLoadGame();
					}
					else
					{
						OnSelectionChanged(save.Initial.game != null);
					}
				}
			}, -1f, 48f);
			dialogGUIToggleButton.guiStyle = skin.customStyles[11];
			dialogGUIToggleButton.OptionInteractableCondition = () => save.Initial.game != null && save.Initial.game.compatible;
			DialogGUIVerticalLayout dialogGUIVerticalLayout = new DialogGUIVerticalLayout(true, false, 0f, new RectOffset(0, 0, 2, 2), TextAnchor.UpperLeft);
			CreateWidget(save, dialogGUIVerticalLayout);
			dialogGUIToggleButton.AddChild(new DialogGUIHorizontalLayout(false, false, 1f, new RectOffset(4, 12, 4, 4), TextAnchor.UpperLeft, new DialogGUIImage(Vector2.one * 44f, Vector2.one * 4f, Color.white, texture), dialogGUIVerticalLayout));
			items.Add(dialogGUIToggleButton);
		}
		count = items.Count;
		for (int j = 0; j < count; j++)
		{
			DialogGUIToggleButton dialogGUIToggleButton2 = items[j];
			Stack<Transform> layouts = new Stack<Transform>();
			layouts.Push(base.transform);
			dialogGUIToggleButton2.Create(ref layouts, skin).transform.SetParent(scrollListContent, worldPositionStays: false);
			dialogGUIToggleButton2.toggle.group = listGroup;
		}
	}

	protected void CreateWidget(ScenarioSaveInfo save, DialogGUIVerticalLayout vLayout)
	{
		DialogGUILabel.TextLabelOptions textLabelOptions = new DialogGUILabel.TextLabelOptions();
		textLabelOptions.resizeBestFit = true;
		textLabelOptions.resizeMaxFontSize = 14;
		textLabelOptions.resizeMinFontSize = 10;
		if (save.Initial.game != null)
		{
			DialogGUILabel dialogGUILabel = new DialogGUILabel((!string.IsNullOrEmpty(save.Initial.game.Title)) ? save.Initial.game.Title : save.name, skin.customStyles[0], expandW: true);
			dialogGUILabel.textLabelOptions = textLabelOptions;
			vLayout.AddChild(dialogGUILabel);
			if (!save.Initial.game.compatible)
			{
				vLayout.AddChild(new DialogGUILabel(Localizer.Format("#autoLOC_465124"), skin.customStyles[1], expandW: true));
			}
		}
		else
		{
			DialogGUILabel dialogGUILabel = new DialogGUILabel(save.name, skin.customStyles[0], expandW: true);
			dialogGUILabel.textLabelOptions = textLabelOptions;
			vLayout.AddChild(dialogGUILabel);
			vLayout.AddChild(new DialogGUILabel(Localizer.Format("#autoLOC_465132"), skin.customStyles[1], expandW: true));
		}
	}

	protected void ClearListItems()
	{
		int count = items.Count;
		while (count-- > 0)
		{
			UnityEngine.Object.Destroy(items[count].uiItem);
		}
		items.Clear();
		selectedGame = null;
		OnSelectionChanged(haveSelection: false);
	}

	protected void SetHidden(bool hide)
	{
		if (hide)
		{
			base.gameObject.SetActive(value: false);
		}
		else
		{
			base.gameObject.SetActive(value: true);
		}
	}

	protected void SetLocked(bool locked)
	{
		int count = items.Count;
		while (count-- > 0)
		{
			items[count].toggle.interactable = !locked;
		}
		btnCancel.interactable = !locked;
		btnRestart.interactable = !locked && selectedGame != null && selectedGame.CanRestart() && scnSet != ScenarioSet.Training;
		btnLoad.interactable = !locked && selectedGame != null;
	}

	private void OnBtnCancel()
	{
		CancelLoadGame();
	}

	private void OnButtonLoad()
	{
		ConfirmLoadGame();
	}

	private void ConfirmLoadGame()
	{
		OnFinishedCallback(selectedGame);
		UnityEngine.Object.Destroy(base.gameObject);
	}

	private void CancelLoadGame()
	{
		OnFinishedCallback(null);
		UnityEngine.Object.Destroy(base.gameObject);
	}

	private void OnButtonRestart()
	{
		confirmGameRestart = true;
		SetLocked(locked: true);
		PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("ScenarioRestart", Localizer.Format("#autoLOC_465205"), Localizer.Format("#autoLOC_465206"), skin, new DialogGUIButton(Localizer.Format("#autoLOC_465208"), ConfirmRestart, dismissOnSelect: true), new DialogGUIButton(Localizer.Format("#autoLOC_465209"), DismissRestartGame, dismissOnSelect: true)), persistAcrossScenes: false, skin).OnDismiss = DismissRestartGame;
	}

	private void ConfirmRestart()
	{
		if (Directory.Exists(selectedGame.resumeFolderFullPath))
		{
			Directory.Delete(selectedGame.resumeFolderFullPath, recursive: true);
		}
		selectedGame.Current.game = selectedGame.Initial.game;
		DismissRestartGame();
	}

	private void DismissRestartGame()
	{
		listGroup.SetAllTogglesOff();
		confirmGameRestart = false;
		selectedGame = null;
		OnSelectionChanged(haveSelection: false);
		SetLocked(locked: false);
	}

	protected void OnSelectionChanged(bool haveSelection)
	{
		btnRestart.interactable = haveSelection && !confirmGameRestart && scnSet != ScenarioSet.Training;
		if (haveSelection)
		{
			btnLoad.interactable = selectedGame.Initial.game.compatible;
			textScnTitle.text = selectedGame.Initial.game.Title;
			textScnDescription.text = selectedGame.Initial.game.Description;
			imgScnBanner.texture = selectedGame.banner;
			imgScnBanner.gameObject.SetActive(imgScnBanner.texture != null);
			btnScnLinkCaption.text = selectedGame.Initial.game.linkCaption;
			btnScnLink.gameObject.SetActive(!string.IsNullOrEmpty(btnScnLinkCaption.text) && !string.IsNullOrEmpty(btnScnLinkCaption.text));
			if (scnSet == ScenarioSet.Training)
			{
				if (Directory.Exists(selectedGame.resumeFolderFullPath))
				{
					Directory.Delete(selectedGame.resumeFolderFullPath, recursive: true);
				}
				selectedGame.Current.game = selectedGame.Initial.game;
			}
			if (selectedGame.CanRestart() && scnSet != ScenarioSet.Training)
			{
				btnLoadCaption.text = Localizer.Format("#autoLOC_465260");
			}
			else
			{
				btnLoadCaption.text = Localizer.Format("#autoLOC_5050040");
			}
			btnRestart.interactable = selectedGame.CanRestart() && scnSet != ScenarioSet.Training;
		}
		else
		{
			btnLoad.interactable = false;
			textScnTitle.text = Localizer.Format("#autoLOC_465267");
			textScnDescription.text = Localizer.Format("#autoLOC_465268");
			imgScnBanner.texture = null;
			imgScnBanner.gameObject.SetActive(value: false);
			btnScnLinkCaption.text = string.Empty;
			btnScnLink.gameObject.SetActive(value: false);
			btnRestart.interactable = false;
		}
	}

	protected void OnBtnScnLink()
	{
		if (selectedGame == null)
		{
			throw new InvalidOperationException("Clicking the ScnLink button should not be possible unless a save is selected and contains a non-empty link url and caption");
		}
		Process.Start(selectedGame.Initial.game.linkURL);
	}
}

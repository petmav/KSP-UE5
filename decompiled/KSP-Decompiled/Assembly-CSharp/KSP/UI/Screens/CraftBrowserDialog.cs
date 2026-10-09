using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using KSP.Localization;
using KSP.UI.Util;
using SaveUpgradePipeline;
using Steamworks;
using TMPro;
using UnityEngine;
using UnityEngine.Networking;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public class CraftBrowserDialog : MonoBehaviour
	{
		public delegate void SelectFileCallback(string fullPath, LoadType t);

		public delegate void SelectConfigNodeCallback(ConfigNode n, LoadType t);

		public delegate void CancelledCallback();

		private enum SteamQueryFilters
		{
			VOTE,
			FEATURED,
			NEWEST,
			SUBSCRIBERS
		}

		public enum LoadType
		{
			Normal,
			Merge
		}

		private DirectoryController directoryController;

		protected List<CraftEntry> craftList;

		protected string craftSubfolder;

		protected EditorFacility facility;

		protected CraftEntry selectedEntry;

		public SelectFileCallback OnFileSelected;

		public SelectConfigNodeCallback OnConfigNodeSelected;

		public CancelledCallback OnBrowseCancelled;

		protected string profile;

		protected bool showMergeOption;

		protected string title;

		[Tooltip("UI to toggle searching in subdirectories")]
		[SerializeField]
		public Toggle subDirectorySearch;

		[SerializeField]
		[Tooltip("UI to toggle searching in all game folders")]
		public Toggle allGameSearch;

		[SerializeField]
		private TextMeshProUGUI header;

		[SerializeField]
		private Toggle tabVAB;

		[SerializeField]
		private Toggle tabSPH;

		[SerializeField]
		private Toggle tabSteam;

		[SerializeField]
		private Button btnCancel;

		[SerializeField]
		private Button btnLoad;

		[SerializeField]
		private Button btnMerge;

		[SerializeField]
		private Button btnDelete;

		[SerializeField]
		private GameObject SteamButtons;

		[SerializeField]
		private Button btnSteamOverlay;

		[SerializeField]
		private Button btnSteamItem;

		[SerializeField]
		private Button btnSteamSubUnsub;

		[SerializeField]
		private TextMeshProUGUI btnSteamSubUnsubText;

		[SerializeField]
		private Button btnMainSteamSubUnsub;

		[SerializeField]
		private TextMeshProUGUI btnMainSteamSubUnsubText;

		[SerializeField]
		private TextMeshProUGUI SteamLoadingText;

		[SerializeField]
		private GameObject SteamFilterHolder;

		[SerializeField]
		private Toggle SteamFilterToggleOne;

		[SerializeField]
		private Toggle SteamFilterToggleTwo;

		[SerializeField]
		private Toggle SteamFilterToggleThree;

		[SerializeField]
		private Toggle SteamFilterToggleFour;

		[SerializeField]
		private UIPanelTransition SteamDetailsPanel;

		[SerializeField]
		private TextMeshProUGUI noSteamItemSelectedText;

		[SerializeField]
		private GameObject SteamItemContent;

		[SerializeField]
		private TextMeshProUGUI AuthorTitleText;

		[SerializeField]
		private TextMeshProUGUI steamItemKSPVersionText;

		[SerializeField]
		private TextMeshProUGUI steamItemVesselTypeText;

		[SerializeField]
		private TextMeshProUGUI steamItemDescriptionText;

		[SerializeField]
		private TextMeshProUGUI steamItemAuthorText;

		[SerializeField]
		private TextMeshProUGUI steamItemModsText;

		[SerializeField]
		private TextMeshProUGUI steamItemFavouriteText;

		[SerializeField]
		private TextMeshProUGUI steamItemFileSizeText;

		[SerializeField]
		private TextMeshProUGUI steamItemUpVotesText;

		[SerializeField]
		private TextMeshProUGUI steamItemDownVotesText;

		[SerializeField]
		private TextMeshProUGUI steamItemSubscribersText;

		[SerializeField]
		private RawImage steamItemPreview;

		[SerializeField]
		private RectTransform scrollView;

		[SerializeField]
		private ToggleGroup listGroup;

		[SerializeField]
		private RectTransform listContainer;

		[SerializeField]
		protected UISkinDefSO uiSkin;

		protected UISkinDef skin;

		private PopupDialog window;

		private PopupDialog unableToSubscribeToSteam;

		private PopupDialog steamError;

		private PopupDialog vesselIsIncompatible;

		private UIConfirmDialog steamWorkshopItemUnsubscribe;

		private List<Selectable> baseSelectables;

		private List<Selectable> totalSelectables;

		private List<Selectable> loadedVesselsSelectables;

		private List<Selectable> filteredLoadedVessels;

		private bool filterNextLoadedVessels;

		[SerializeField]
		private Transform directoryContent;

		private MenuNavigation menuNavigation;

		private Navigation tabVabNav;

		private Navigation tabSphNav;

		private Navigation secondVesselNav;

		private Navigation steamToggleOneNav;

		private Navigation steamToggleTwoNav;

		private Navigation steamToggleThreeNav;

		private Navigation steamToggleFourNav;

		private Navigation btnCancelNav;

		private Navigation btnLoadNav;

		private Navigation btnMergeNav;

		private Navigation btnDeleteNav;

		private Navigation btnSteamOverlayNav;

		[SerializeField]
		private PublishedFileId_t[] steamSubscribedItems;

		private string[] nonGameSaveDirectories = new string[4] { "missions", "scenarios", "test_missions", "training" };

		private SteamQueryFilters selectedSteamQueryFilter;

		private bool steamTabOpen;

		private string authorNormalTitle = "";

		private string authorSteamTitle = "";

		private const string folderSeparator = "/";

		private const string craftFiles = "*.craft";

		public static ScrollRect ScrollView { get; private set; }

		public bool IsSubdirectorySearch
		{
			get
			{
				if (subDirectorySearch != null)
				{
					return subDirectorySearch.isOn;
				}
				return false;
			}
		}

		public bool IsAllGameSearch
		{
			get
			{
				if (allGameSearch != null)
				{
					return allGameSearch.isOn;
				}
				return false;
			}
		}

		public static CraftBrowserDialog Spawn(EditorFacility facility, string profile, SelectFileCallback onFileSelected, CancelledCallback onCancel, bool showMergeOption)
		{
			CraftBrowserDialog component = UnityEngine.Object.Instantiate(AssetBase.GetPrefab("CraftBrowser")).GetComponent<CraftBrowserDialog>();
			component.transform.SetParent(DialogCanvasUtil.DialogCanvasRect, worldPositionStays: false);
			component.facility = facility;
			component.showMergeOption = showMergeOption;
			component.OnBrowseCancelled = onCancel;
			component.OnFileSelected = onFileSelected;
			component.title = Localizer.Format("#autoLOC_900537");
			component.profile = profile;
			return component;
		}

		public static CraftBrowserDialog Spawn(EditorFacility facility, string profile, SelectConfigNodeCallback onConfigNodeSelected, CancelledCallback onCancel, bool showMergeOption)
		{
			CraftBrowserDialog component = UnityEngine.Object.Instantiate(AssetBase.GetPrefab("CraftBrowser")).GetComponent<CraftBrowserDialog>();
			component.transform.SetParent(DialogCanvasUtil.DialogCanvasRect, worldPositionStays: false);
			component.facility = facility;
			component.showMergeOption = showMergeOption;
			component.OnBrowseCancelled = onCancel;
			component.OnConfigNodeSelected = onConfigNodeSelected;
			component.title = Localizer.Format("#autoLOC_900537");
			component.profile = profile;
			return component;
		}

		public void Dismiss()
		{
			if (SteamManager.Initialized)
			{
				SteamManager.Instance.RemoveRemoteSubUnsubEvents();
			}
			for (int i = 0; i < craftList.Count; i++)
			{
				UnityEngine.Object.Destroy(craftList[i]);
			}
			UnityEngine.Object.Destroy(base.gameObject);
		}

		internal void HideForLater()
		{
			menuNavigation.SetMenuNavInputLock(newState: false);
			base.gameObject.SetActive(value: false);
			CraftSearch.Instance.OnDisable();
		}

		internal void ReDisplay(EditorFacility newFacility, bool showMergeOption)
		{
			facility = newFacility;
			tabVAB.isOn = facility == EditorFacility.VAB;
			tabSPH.isOn = facility == EditorFacility.SPH;
			this.showMergeOption = showMergeOption && !DirectoryController.IsTrainingScenario;
			if (showMergeOption)
			{
				btnMerge.onClick.AddListener(onButtonMerge);
			}
			else
			{
				btnMerge.gameObject.SetActive(value: false);
			}
			craftSubfolder = ShipConstruction.GetShipsSubfolderFor(facility);
			base.gameObject.SetActive(value: true);
			menuNavigation.SetMenuNavInputLock(newState: true);
			directoryController.ShowDirectoryTreeForEditor(facility);
			BuildPlayerCraftList();
		}

		public void Start()
		{
			baseSelectables = new List<Selectable>();
			totalSelectables = new List<Selectable>();
			loadedVesselsSelectables = new List<Selectable>();
			filteredLoadedVessels = new List<Selectable>();
			ScrollView = scrollView.GetComponent<ScrollRect>();
			header.text = title;
			tabVAB.isOn = facility == EditorFacility.VAB;
			tabVAB.onValueChanged.AddListener(onVABtabToggle);
			tabSPH.isOn = facility == EditorFacility.SPH;
			tabSPH.onValueChanged.AddListener(onSPHtabToggle);
			if (SteamManager.Initialized)
			{
				tabSteam.gameObject.SetActive(value: true);
				tabSteam.isOn = false;
				tabSteam.onValueChanged.AddListener(onSteamtabToggle);
				btnSteamSubUnsub.onClick.AddListener(onButtonSteamSubUnsub);
				btnMainSteamSubUnsub.onClick.AddListener(onButtonSteamSubUnsub);
				SteamFilterToggleOne.onValueChanged.AddListener(onSteamFilterOne);
				SteamFilterToggleTwo.onValueChanged.AddListener(onSteamFilterTwo);
				SteamFilterToggleThree.onValueChanged.AddListener(onSteamFilterThree);
				SteamFilterToggleFour.onValueChanged.AddListener(onSteamFilterFour);
				btnSteamOverlay.onClick.AddListener(OnBtnSteamOverlay);
				btnSteamItem.onClick.AddListener(onBtnSteamItemDetails);
				UpdateSteamSubscribedItems();
			}
			if (showMergeOption)
			{
				btnMerge.onClick.AddListener(onButtonMerge);
			}
			else
			{
				btnMerge.gameObject.SetActive(value: false);
			}
			btnDelete.gameObject.SetActive(!DirectoryController.IsTrainingScenario);
			btnDelete.onClick.AddListener(onButtonDelete);
			btnCancel.onClick.AddListener(onButtonCancel);
			btnLoad.onClick.AddListener(onButtonLoad);
			subDirectorySearch.enabled = !HighLogic.CurrentGame.IsMissionMode;
			allGameSearch.enabled = !HighLogic.CurrentGame.IsMissionMode;
			if (HighLogic.CurrentGame.IsMissionMode)
			{
				subDirectorySearch.isOn = false;
				allGameSearch.isOn = false;
			}
			else
			{
				subDirectorySearch.onValueChanged.AddListener(SearchOption_OnChanged);
				allGameSearch.onValueChanged.AddListener(SearchOption_OnChanged);
			}
			craftSubfolder = ShipConstruction.GetShipsSubfolderFor(facility);
			Selectable[] componentsInChildren = directoryContent.GetComponentsInChildren<Selectable>();
			for (int i = 0; i < componentsInChildren.Length; i++)
			{
				baseSelectables.Add(componentsInChildren[i]);
			}
			baseSelectables.Add(btnDelete);
			baseSelectables.Add(btnSteamOverlay);
			craftSubfolder = ShipConstruction.GetShipsSubfolderFor(facility);
			directoryController = base.gameObject.GetComponent<DirectoryController>();
			directoryController.ShowDirectoryTreeForEditor(facility);
			BuildPlayerCraftList();
			authorNormalTitle = Localizer.Format("#autoLOC_8006112");
			authorSteamTitle = Localizer.Format("#autoLOC_8002161");
			InitializeMenuNavigation();
			UpdateSelectables();
			UpdateExplicitNavigation();
			if (CraftSearch.Instance != null)
			{
				CraftSearch instance = CraftSearch.Instance;
				instance.hasFiltered = (CraftSearch.HasFiltered)Delegate.Combine(instance.hasFiltered, new CraftSearch.HasFiltered(FilterUpdateSelectables));
			}
		}

		public void Update()
		{
			if (Input.GetKeyUp(KeyCode.BackQuote))
			{
				if (!(PartCategorizer.Instance != null) || !PartCategorizer.Instance.searchField.isFocused)
				{
					InputLockManager.SetControlLock(ControlTypes.KEYBOARDINPUT, "CraftSearchFieldTextInput");
					CraftSearch.Instance.searchField.ActivateInputField();
				}
			}
			else if (Input.GetKeyUp(KeyCode.Escape) && !unableToSubscribeToSteam && !steamError && !vesselIsIncompatible && !steamWorkshopItemUnsubscribe)
			{
				if (!string.IsNullOrWhiteSpace(CraftSearch.Instance.searchField.text))
				{
					CraftSearch.Instance.searchField.text = string.Empty;
					InputLockManager.RemoveControlLock("CraftSearchFieldTextInput");
				}
				else
				{
					onButtonCancel();
				}
			}
		}

		public void OnDisable()
		{
			CraftEntry craftEntry = selectedEntry;
			ClearCraftList();
			selectedEntry = craftEntry;
		}

		private void OnDestroy()
		{
			if (CraftSearch.Instance != null)
			{
				CraftSearch instance = CraftSearch.Instance;
				instance.hasFiltered = (CraftSearch.HasFiltered)Delegate.Remove(instance.hasFiltered, new CraftSearch.HasFiltered(FilterUpdateSelectables));
			}
		}

		private void Show()
		{
			base.gameObject.SetActive(value: true);
		}

		protected void Hide()
		{
			base.gameObject.SetActive(value: false);
		}

		internal static CraftEntry RemoveCreateCraftEntry(List<CraftEntry> craftEntries, FileInfo file, bool stock, Callback<CraftEntry> OnSelected, bool steamItem, SteamCraftInfo steamCraftInfo)
		{
			CraftEntry craftEntry = null;
			if (craftEntries != null)
			{
				for (int i = 0; i < craftEntries.Count; i++)
				{
					if (craftEntries[i].fullFilePath == file.FullName && craftEntries[i].craftProfileInfo.lastWriteTime == file.LastWriteTimeUtc.Ticks)
					{
						craftEntry = craftEntries[i];
						craftEntries.RemoveAt(i);
						break;
					}
				}
			}
			if (craftEntry == null)
			{
				craftEntry = CraftEntry.Create(file, stock, OnSelected, steamItem, steamCraftInfo);
			}
			return craftEntry;
		}

		public void BuildPlayerCraftList()
		{
			SteamLoadingText.gameObject.SetActive(value: false);
			List<CraftEntry> list = craftList;
			CraftProfileInfo.PrepareCraftMetaFileLoad();
			craftList = new List<CraftEntry>();
			if (profile != null && profile != string.Empty)
			{
				FileInfo[] array = new FileInfo[0];
				Game.Modes mode = HighLogic.CurrentGame.Mode;
				array = (((uint)(mode - 5) > 1u) ? GetPlayerCraftFiles() : GetMissionCraftFiles());
				int i = 0;
				for (int num = array.Length; i < num; i++)
				{
					CraftEntry craftEntry = RemoveCreateCraftEntry(list, array[i], stock: false, OnEntrySelected, steamItem: false, null);
					craftEntry.SetDirectoryController(directoryController);
					craftEntry.ShowPath(IsSubdirectorySearch || IsAllGameSearch);
					craftList.Add(craftEntry);
				}
			}
			ClearOldCraftEntries(list);
			UpdateCraftListUI(craftList);
			UpdateSelectables();
			Canvas.ForceUpdateCanvases();
		}

		private FileInfo[] GetMissionCraftFiles()
		{
			FileInfo[] result = new FileInfo[0];
			string missionCraftPath = directoryController.MissionCraftPath;
			if (!string.IsNullOrWhiteSpace(missionCraftPath))
			{
				DirectoryInfo directoryInfo = new DirectoryInfo(missionCraftPath + craftSubfolder);
				if (directoryInfo.Exists)
				{
					result = directoryInfo.GetFiles("*.craft");
				}
			}
			return result;
		}

		private FileInfo[] GetPlayerCraftFiles()
		{
			FileInfo[] result = new FileInfo[0];
			bool isSubdirectorySearch = IsSubdirectorySearch;
			bool isAllGameSearch = IsAllGameSearch;
			try
			{
				if (isSubdirectorySearch && isAllGameSearch)
				{
					return GetAllGameFiles("*.craft", SearchOption.AllDirectories);
				}
				if (isAllGameSearch && !isSubdirectorySearch)
				{
					return GetAllGameFiles("*.craft", SearchOption.TopDirectoryOnly);
				}
				if (isSubdirectorySearch && !isAllGameSearch)
				{
					DirectoryInfo directoryInfo = new DirectoryInfo(directoryController.CurrentSelectedDirectoryPath);
					if (!directoryInfo.Exists)
					{
						directoryInfo.Create();
					}
					return directoryInfo.GetFiles("*.craft", SearchOption.AllDirectories);
				}
				DirectoryInfo directoryInfo2 = new DirectoryInfo(directoryController.CurrentSelectedDirectoryPath);
				if (!directoryInfo2.Exists)
				{
					directoryInfo2.Create();
				}
				return directoryInfo2.GetFiles("*.craft");
			}
			catch (Exception)
			{
				return result;
			}
		}

		private FileInfo[] GetAllGameFiles(string searchPattern, SearchOption searchOption)
		{
			DirectoryInfo directoryInfo = new DirectoryInfo(ShipConstruction.GetShipsPathFor(HighLogic.SaveFolder));
			if (directoryInfo == null)
			{
				return null;
			}
			DirectoryInfo parent = directoryInfo.Parent;
			if (parent == null)
			{
				return null;
			}
			DirectoryInfo[] directories = parent.GetDirectories();
			if (directories == null)
			{
				return null;
			}
			List<FileInfo> list = new List<FileInfo>();
			int i = 0;
			for (int num = directories.Length; i < num; i++)
			{
				DirectoryInfo directoryInfo2 = directories[i];
				if (nonGameSaveDirectories.IndexOf(directoryInfo2.Name) <= -1)
				{
					DirectoryInfo directoryInfo3 = new DirectoryInfo(ShipConstruction.GetShipsPathFor(directories[i].Name, facility));
					if (!directoryInfo3.Exists)
					{
						directoryInfo3.Create();
					}
					FileInfo[] files = directoryInfo3.GetFiles("*.craft", searchOption);
					int j = 0;
					for (int num2 = files.Length; j < num2; j++)
					{
						list.Add(files[j]);
					}
				}
			}
			return list.ToArray();
		}

		public void BuildStockCraftList()
		{
			SteamLoadingText.gameObject.SetActive(value: false);
			List<CraftEntry> list = craftList;
			CraftProfileInfo.PrepareCraftMetaFileLoad();
			craftList = new List<CraftEntry>();
			if (DirectoryController.IsStockCraftAvailable)
			{
				DirectoryInfo directoryInfo = new DirectoryInfo(directoryController.StockDirectoryPath + craftSubfolder);
				if (directoryInfo.Exists)
				{
					FileInfo[] files = directoryInfo.GetFiles("*.craft");
					int num = files.Length;
					for (int i = 0; i < num; i++)
					{
						CraftEntry item = RemoveCreateCraftEntry(list, files[i], stock: true, OnEntrySelected, steamItem: false, null);
						craftList.Add(item);
					}
				}
				List<string> list2 = directoryController.ExpansionDirectories();
				int j = 0;
				for (int count = list2.Count; j < count; j++)
				{
					DirectoryInfo directoryInfo2 = new DirectoryInfo(list2[j] + ShipConstruction.GetShipsSubfolderFor(facility) + "/");
					if (directoryInfo2.Exists)
					{
						FileInfo[] files2 = directoryInfo2.GetFiles("*.craft");
						int num = files2.Length;
						for (int k = 0; k < num; k++)
						{
							CraftEntry item2 = RemoveCreateCraftEntry(list, files2[k], stock: true, OnEntrySelected, steamItem: false, null);
							craftList.Add(item2);
						}
					}
				}
				if (IsAllGameSearch)
				{
					FileInfo[] playerCraftFiles = GetPlayerCraftFiles();
					int num = playerCraftFiles.Length;
					for (int l = 0; l < num; l++)
					{
						CraftEntry craftEntry = RemoveCreateCraftEntry(list, playerCraftFiles[l], stock: false, OnEntrySelected, steamItem: false, null);
						craftEntry.ShowPath(show: true);
						craftList.Add(craftEntry);
					}
				}
			}
			ClearOldCraftEntries(list);
			UpdateCraftListUI(craftList);
			UpdateSelectables();
		}

		public void BuildSteamSubscribedCraftList()
		{
			SteamLoadingText.gameObject.SetActive(value: false);
			List<CraftEntry> list = craftList;
			CraftProfileInfo.PrepareCraftMetaFileLoad();
			craftList = new List<CraftEntry>();
			if (SteamManager.Initialized && !tabSteam.isOn)
			{
				craftList.AddRange(KSPSteamUtils.GatherCraftFiles(list, OnEntrySelected, excludeSteamUnsubscribed: true));
			}
			ClearOldCraftEntries(list);
			UpdateCraftListUI(craftList);
			UpdateSelectables();
			Canvas.ForceUpdateCanvases();
		}

		private void UpdateCraftListUI(List<CraftEntry> craftList)
		{
			if (loadedVesselsSelectables != null)
			{
				loadedVesselsSelectables.Clear();
			}
			craftList = craftList.OrderByAlphaNumeric((CraftEntry c) => c.craftName);
			int count = craftList.Count;
			for (int i = 0; i < count; i++)
			{
				AddCraftEntryWidget(craftList[i], listContainer);
				if (craftList[i] != listContainer.GetChild(i))
				{
					craftList[i].transform.SetSiblingIndex(i);
				}
			}
		}

		private void FilterUpdateSelectables(bool filtered)
		{
			filterNextLoadedVessels = filtered;
			UpdateSelectables();
		}

		private void UpdateSelectables()
		{
			if (!(menuNavigation != null))
			{
				return;
			}
			menuNavigation.ResetSelectablesOnly();
			totalSelectables.Clear();
			totalSelectables.AddRange(baseSelectables);
			filteredLoadedVessels.Clear();
			filteredLoadedVessels.AddRange(loadedVesselsSelectables);
			if (filterNextLoadedVessels)
			{
				for (int i = 0; i < filteredLoadedVessels.Count; i++)
				{
					if (!filteredLoadedVessels[i].gameObject.activeInHierarchy)
					{
						filteredLoadedVessels.RemoveAt(i);
						i--;
					}
				}
				filterNextLoadedVessels = false;
			}
			totalSelectables.AddRange(filteredLoadedVessels);
			if (SteamManager.Initialized && steamTabOpen)
			{
				totalSelectables.Add(SteamFilterToggleOne);
				totalSelectables.Add(SteamFilterToggleTwo);
				totalSelectables.Add(SteamFilterToggleThree);
				totalSelectables.Add(SteamFilterToggleFour);
			}
			if (totalSelectables.Count > 0)
			{
				menuNavigation.SetSelectableItems(totalSelectables.ToArray(), Navigation.Mode.Automatic, hasText: false, resetNavMode: true);
			}
			UpdateExplicitNavigation();
		}

		private void InitializeMenuNavigation()
		{
			menuNavigation = MenuNavigation.SpawnMenuNavigation(directoryContent.gameObject, Navigation.Mode.Automatic, SliderFocusType.Scrollbar, hasText: false, limitCheck: true);
			menuNavigation.SetSearchField();
			tabVabNav = new Navigation
			{
				mode = Navigation.Mode.Explicit,
				selectOnRight = tabSPH,
				selectOnUp = btnCancel
			};
			tabVAB.navigation = tabVabNav;
			tabSphNav = new Navigation
			{
				mode = Navigation.Mode.Explicit,
				selectOnLeft = tabVAB,
				selectOnRight = tabSteam,
				selectOnUp = btnCancel
			};
			tabVAB.navigation = tabSphNav;
			secondVesselNav = new Navigation
			{
				mode = Navigation.Mode.Explicit
			};
			btnLoadNav = new Navigation
			{
				mode = Navigation.Mode.Explicit,
				selectOnDown = tabSPH,
				selectOnLeft = btnCancel
			};
			btnCancelNav = new Navigation
			{
				mode = Navigation.Mode.Explicit,
				selectOnDown = tabSPH,
				selectOnLeft = btnMerge,
				selectOnRight = btnLoad
			};
			btnMergeNav = new Navigation
			{
				mode = Navigation.Mode.Explicit,
				selectOnDown = tabVAB,
				selectOnRight = btnCancel
			};
			btnDeleteNav = new Navigation
			{
				mode = Navigation.Mode.Explicit,
				selectOnDown = tabVAB,
				selectOnRight = btnCancel,
				selectOnLeft = btnMerge
			};
			if (SteamManager.Initialized)
			{
				btnSteamOverlayNav = new Navigation
				{
					mode = Navigation.Mode.Explicit,
					selectOnDown = tabSPH,
					selectOnRight = btnCancel
				};
				steamToggleOneNav = new Navigation
				{
					mode = Navigation.Mode.Explicit,
					selectOnRight = SteamFilterToggleTwo,
					selectOnUp = tabVAB
				};
				SteamFilterToggleOne.navigation = steamToggleOneNav;
				steamToggleTwoNav = new Navigation
				{
					mode = Navigation.Mode.Explicit,
					selectOnLeft = SteamFilterToggleOne,
					selectOnRight = SteamFilterToggleThree,
					selectOnUp = tabSPH
				};
				SteamFilterToggleTwo.navigation = steamToggleTwoNav;
				steamToggleThreeNav = new Navigation
				{
					mode = Navigation.Mode.Explicit,
					selectOnLeft = SteamFilterToggleTwo,
					selectOnRight = SteamFilterToggleFour,
					selectOnUp = tabSPH
				};
				SteamFilterToggleThree.navigation = steamToggleThreeNav;
				steamToggleFourNav = new Navigation
				{
					mode = Navigation.Mode.Explicit,
					selectOnLeft = SteamFilterToggleThree,
					selectOnUp = tabSteam
				};
				SteamFilterToggleFour.navigation = steamToggleFourNav;
			}
		}

		private void UpdateExplicitNavigation()
		{
			if (filteredLoadedVessels.Count <= 0)
			{
				return;
			}
			if (SteamManager.Initialized && steamTabOpen)
			{
				steamToggleOneNav.selectOnDown = filteredLoadedVessels[0];
				steamToggleTwoNav.selectOnDown = filteredLoadedVessels[0];
				steamToggleThreeNav.selectOnDown = filteredLoadedVessels[0];
				steamToggleFourNav.selectOnDown = filteredLoadedVessels[0];
				btnSteamOverlayNav.selectOnUp = filteredLoadedVessels[filteredLoadedVessels.Count - 1];
				btnCancelNav.selectOnUp = filteredLoadedVessels[filteredLoadedVessels.Count - 1];
				btnCancelNav.selectOnLeft = btnSteamOverlay;
				btnCancelNav.selectOnRight = btnSteamItem;
				SteamFilterToggleOne.navigation = steamToggleOneNav;
				SteamFilterToggleTwo.navigation = steamToggleTwoNav;
				SteamFilterToggleThree.navigation = steamToggleThreeNav;
				SteamFilterToggleFour.navigation = steamToggleFourNav;
				tabVabNav.selectOnDown = SteamFilterToggleOne;
				tabVabNav.selectOnUp = btnSteamOverlay;
				tabSphNav.selectOnDown = SteamFilterToggleTwo;
				tabSphNav.selectOnUp = btnCancel;
				tabSphNav.selectOnLeft = btnSteamOverlay;
				btnSteamOverlay.navigation = btnSteamOverlayNav;
				btnCancel.navigation = btnCancelNav;
				tabVAB.navigation = tabVabNav;
				tabSPH.navigation = tabSphNav;
			}
			else
			{
				tabVabNav.selectOnDown = filteredLoadedVessels[0];
				tabVabNav.selectOnUp = btnMerge;
				tabSphNav.selectOnDown = filteredLoadedVessels[0];
				tabSphNav.selectOnUp = btnCancel;
				btnCancelNav.selectOnUp = filteredLoadedVessels[filteredLoadedVessels.Count - 1];
				btnCancelNav.selectOnDown = tabSPH;
				btnCancelNav.selectOnLeft = btnMerge;
				btnCancelNav.selectOnRight = btnLoad;
				btnLoadNav.selectOnUp = filteredLoadedVessels[filteredLoadedVessels.Count - 1];
				btnMergeNav.selectOnUp = filteredLoadedVessels[filteredLoadedVessels.Count - 1];
				btnDeleteNav.selectOnUp = filteredLoadedVessels[filteredLoadedVessels.Count - 1];
				if (btnDelete.gameObject.activeInHierarchy)
				{
					btnMergeNav.selectOnRight = btnDelete;
					btnCancelNav.selectOnLeft = btnDelete;
				}
				tabVAB.navigation = tabVabNav;
				tabSPH.navigation = tabSphNav;
				btnLoad.navigation = btnLoadNav;
				btnCancel.navigation = btnCancelNav;
				btnMerge.navigation = btnMergeNav;
				btnDelete.navigation = btnDeleteNav;
			}
			if (filteredLoadedVessels.Count >= 2)
			{
				secondVesselNav.selectOnUp = filteredLoadedVessels[0];
				if (filteredLoadedVessels.Count >= 3)
				{
					secondVesselNav.selectOnDown = filteredLoadedVessels[2];
				}
				filteredLoadedVessels[1].navigation = secondVesselNav;
			}
		}

		protected void AddCraftEntryWidget(CraftEntry entry, RectTransform listParent)
		{
			listGroup.RegisterToggle(entry.Toggle);
			entry.Toggle.group = listGroup;
			entry.transform.SetParent(listParent, worldPositionStays: false);
			loadedVesselsSelectables.Add(entry.Toggle);
		}

		protected void ClearCraftList()
		{
			loadedVesselsSelectables.Clear();
			if (craftList != null)
			{
				int count = craftList.Count;
				while (count-- > 0)
				{
					ClearCraftEntry(craftList[count]);
				}
				craftList.Clear();
				selectedEntry = null;
			}
			OnSelectionChanged(null);
		}

		protected void ClearOldCraftEntries(List<CraftEntry> oldCraftList)
		{
			if (oldCraftList != null)
			{
				int count = oldCraftList.Count;
				while (count-- > 0)
				{
					ClearCraftEntry(oldCraftList[count]);
				}
			}
		}

		protected void ClearCraftEntry(CraftEntry craftEntry)
		{
			listGroup.UnregisterToggle(craftEntry.Toggle);
			craftEntry.Terminate();
		}

		protected void ClearSelection()
		{
			selectedEntry = null;
			OnSelectionChanged(null);
			listGroup.SetAllTogglesOff();
		}

		protected void OnEntrySelected(CraftEntry entry)
		{
			OnSelectionChanged(entry);
			if (selectedEntry == entry && Mouse.Left.GetDoubleClick(isDelegate: true))
			{
				pipeSelectedItem(selectedEntry, LoadType.Normal);
				HideForLater();
			}
			else
			{
				selectedEntry = entry;
			}
		}

		protected void OnSelectionChanged(CraftEntry selectedEntry)
		{
			setbottomButtons(selectedEntry, steamTabOpen);
			if (selectedEntry != null)
			{
				btnMerge.interactable = true;
				btnLoad.interactable = true;
				btnSteamSubUnsub.interactable = false;
				btnSteamItem.interactable = false;
				AuthorTitleText.text = authorNormalTitle;
				if (selectedEntry.steamItem && selectedEntry.steamCraftInfo != null)
				{
					AuthorTitleText.text = authorSteamTitle;
					noSteamItemSelectedText.gameObject.SetActive(value: false);
					SteamItemContent.SetActive(value: true);
					steamItemPreview.gameObject.SetActive(value: false);
					StartCoroutine(LoadSteamItemPreviewURL(selectedEntry));
					bool flag = SteamItemSubscribed(selectedEntry.steamCraftInfo.itemDetails.m_nPublishedFileId.m_PublishedFileId);
					btnSteamSubUnsub.interactable = true;
					btnSteamItem.interactable = true;
					btnSteamSubUnsubText.text = (flag ? Localizer.Format("#autoLOC_8002135") : Localizer.Format("#autoLOC_8002134"));
					steamItemKSPVersionText.text = selectedEntry.steamCraftInfo.KSPversion;
					steamItemVesselTypeText.text = selectedEntry.steamCraftInfo.vesselType;
					steamItemDescriptionText.text = selectedEntry.steamCraftInfo.itemDetails.m_rgchDescription;
					steamItemAuthorText.text = SteamManager.Instance.getUserName(selectedEntry.steamCraftInfo.itemDetails.m_ulSteamIDOwner, steamItemAuthorText);
					steamItemFavouriteText.text = selectedEntry.steamCraftInfo.totalFavorites.ToString();
					float num = (float)selectedEntry.steamCraftInfo.itemDetails.m_nFileSize / 1024f / 1024f;
					steamItemFileSizeText.text = num.ToString("N2");
					steamItemModsText.text = selectedEntry.steamCraftInfo.modsBriefing;
					steamItemDownVotesText.text = selectedEntry.steamCraftInfo.itemDetails.m_unVotesDown.ToString();
					steamItemUpVotesText.text = selectedEntry.steamCraftInfo.itemDetails.m_unVotesUp.ToString();
					steamItemSubscribersText.text = selectedEntry.steamCraftInfo.totalSubscriptions.ToString();
				}
				if (selectedEntry.steamItem && !steamTabOpen)
				{
					btnMainSteamSubUnsub.gameObject.SetActive(value: true);
					ulong steamFileId = selectedEntry.GetSteamFileId();
					if (steamFileId == 0L)
					{
						btnMainSteamSubUnsub.interactable = false;
						return;
					}
					btnMainSteamSubUnsub.interactable = true;
					bool flag2 = SteamItemSubscribed(steamFileId);
					btnMainSteamSubUnsubText.text = (flag2 ? Localizer.Format("#autoLOC_8002135") : Localizer.Format("#autoLOC_8002134"));
				}
				else
				{
					btnMainSteamSubUnsub.gameObject.SetActive(value: false);
					btnMainSteamSubUnsub.interactable = false;
				}
			}
			else
			{
				btnMerge.interactable = false;
				btnLoad.interactable = false;
				btnSteamSubUnsub.interactable = false;
				noSteamItemSelectedText.gameObject.SetActive(value: true);
				SteamItemContent.SetActive(value: false);
				btnSteamItem.interactable = false;
				btnMainSteamSubUnsub.gameObject.SetActive(value: false);
				btnMainSteamSubUnsub.interactable = false;
			}
		}

		private IEnumerator LoadSteamItemPreviewURL(CraftEntry selectedEntry)
		{
			UnityWebRequest www = UnityWebRequestTexture.GetTexture(selectedEntry.steamCraftInfo.previewURL);
			yield return www.SendWebRequest();
			while (!www.isDone)
			{
				yield return null;
			}
			if (!www.isNetworkError && !www.isHttpError)
			{
				steamItemPreview.texture = DownloadHandlerTexture.GetContent(www);
				steamItemPreview.gameObject.SetActive(value: true);
			}
			else
			{
				Debug.LogWarning("Texture load error in '" + selectedEntry.craftName + "': " + www.error);
			}
		}

		protected void onButtonLoad()
		{
			if (selectedEntry != null)
			{
				if (selectedEntry.compatibility == VersionCompareResult.COMPATIBLE && selectedEntry.isValid)
				{
					ConfirmLoadCraft();
					return;
				}
				if (selectedEntry.craftProfileInfo.shipPartsUnlocked && selectedEntry.craftProfileInfo.shipPartModulesAvailable)
				{
					vesselIsIncompatible = PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("Confirmation Needed", Localizer.Format("#autoLOC_8004239"), Localizer.Format("#autoLOC_464288"), skin, 350f, new DialogGUIButton(Localizer.Format("#autoLOC_417274"), ConfirmLoadCraft, dismissOnSelect: true), new DialogGUIButton(Localizer.Format("#autoLOC_226976"), null, dismissOnSelect: true)), persistAcrossScenes: false, skin);
					return;
				}
				DialogGUILabel dialogGUILabel = new DialogGUILabel(selectedEntry.craftProfileInfo.GetErrorMessage());
				dialogGUILabel.bypassTextStyleColor = true;
				DialogGUIVerticalLayout dialogGUIVerticalLayout = new DialogGUIVerticalLayout(true, false, 0f, new RectOffset(4, 11, 4, 4), TextAnchor.UpperLeft, dialogGUILabel);
				dialogGUIVerticalLayout.AddChild(new DialogGUIContentSizer(ContentSizeFitter.FitMode.Unconstrained, ContentSizeFitter.FitMode.PreferredSize));
				PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("Confirmation Needed", Localizer.Format("#autoLOC_8004267"), Localizer.Format("#autoLOC_464288"), skin, 350f, new DialogGUISpace(6f), new DialogGUIScrollList(new Vector2(-1f, 80f), hScroll: false, vScroll: true, dialogGUIVerticalLayout), new DialogGUISpace(6f), new DialogGUIHorizontalLayout(TextAnchor.UpperLeft, new DialogGUIFlexibleSpace(), new DialogGUIButton(Localizer.Format("#autoLOC_417274"), ConfirmLoadCraft, 100f, -1f, true), new DialogGUIButton(Localizer.Format("#autoLOC_226976"), null, 150f, -1f, true))), persistAcrossScenes: false, skin);
			}
		}

		protected void ConfirmLoadCraft()
		{
			if (!selectedEntry.craftProfileInfo.shipPartsUnlocked || !selectedEntry.craftProfileInfo.shipPartModulesAvailable)
			{
				Debug.LogWarning("Loading craft with the following issues:\n" + selectedEntry.craftProfileInfo.GetErrorMessage());
			}
			pipeSelectedItem(selectedEntry, LoadType.Normal);
			HideForLater();
		}

		private void onButtonMerge()
		{
			if (selectedEntry != null && base.gameObject.activeInHierarchy)
			{
				pipeSelectedItem(selectedEntry, LoadType.Merge);
				HideForLater();
			}
		}

		protected void onButtonCancel()
		{
			OnBrowseCancelled();
			HideForLater();
		}

		private void SearchOption_OnChanged(bool value)
		{
			CraftSearch.Instance.StopSearch();
			if (directoryController.IsPlayerCraftDirectorySelected)
			{
				BuildPlayerCraftList();
			}
			if (directoryController.IsStockDirectorySelected)
			{
				BuildStockCraftList();
			}
			if (directoryController.IsSteamDirectorySelected)
			{
				BuildSteamSubscribedCraftList();
			}
		}

		private void UpdateSteamSubscribedItems()
		{
			if (SteamManager.Initialized)
			{
				uint numberSubscribedItems = SteamManager.Instance.GetNumberSubscribedItems();
				steamSubscribedItems = SteamManager.Instance.GetSubscribedItems(numberSubscribedItems);
			}
		}

		private bool SteamItemSubscribed(ulong fileId)
		{
			int num = 0;
			while (true)
			{
				if (num < steamSubscribedItems.Length)
				{
					if (steamSubscribedItems[num].m_PublishedFileId == fileId)
					{
						break;
					}
					num++;
					continue;
				}
				return false;
			}
			return true;
		}

		private void UpdateSteamListOfCraft(SteamQueryFilters selectedQueryType)
		{
			ClearCraftList();
			EUGCQuery eUGCQuery = EUGCQuery.k_EUGCQuery_RankedByVote;
			string[] tags = new string[1] { "Craft" };
			switch (selectedQueryType)
			{
			default:
				eUGCQuery = EUGCQuery.k_EUGCQuery_RankedByVote;
				break;
			case SteamQueryFilters.FEATURED:
				eUGCQuery = EUGCQuery.k_EUGCQuery_RankedByVote;
				tags = new string[2] { "Craft", "Featured" };
				break;
			case SteamQueryFilters.NEWEST:
				eUGCQuery = EUGCQuery.k_EUGCQuery_RankedByPublicationDate;
				break;
			case SteamQueryFilters.SUBSCRIBERS:
				eUGCQuery = EUGCQuery.k_EUGCQuery_RankedByTotalUniqueSubscriptions;
				break;
			}
			getSteamWorkshopCraft(eUGCQuery, tags);
		}

		private void getSteamWorkshopCraft(EUGCQuery queryType, string[] tags)
		{
			SteamManager.Instance.QueryUGCItems(onSteamQueryItemsCallback, queryType, EUGCMatchingUGCType.k_EUGCMatchingUGCType_Items_ReadyToUse, tags, 1u, returnMetadata: true);
		}

		private void onSteamQueryItemsCallback(SteamUGCQueryCompleted_t results, bool bIOFailure)
		{
			if (bIOFailure && results.m_eResult == EResult.k_EResultOK)
			{
				AnalyticsUtil.LogSteamError(AnalyticsUtil.steamActions.query, AnalyticsUtil.steamItemTypes.craft, 0uL, results.m_eResult);
				steamError = PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("SteamError", Localizer.Format("#autoLOC_8002168", SteamManager.Instance.GetUGCFailureReason(results.m_eResult)), Localizer.Format("#autoLOC_8002125"), skin, 350f, new DialogGUIButton(Localizer.Format("#autoLOC_226975"), null, dismissOnSelect: true)), persistAcrossScenes: false, skin);
				SteamUGC.ReleaseQueryUGCRequest(results.m_handle);
				return;
			}
			SteamLoadingText.gameObject.SetActive(value: false);
			if (steamTabOpen)
			{
				loadedVesselsSelectables.Clear();
				for (int i = 0; i < results.m_unNumResultsReturned; i++)
				{
					SteamUGCDetails_t uGCItemDetails = SteamManager.Instance.GetUGCItemDetails(results.m_handle, (uint)i);
					if (uGCItemDetails.m_rgchTags.Contains("Craft") && (selectedSteamQueryFilter != SteamQueryFilters.FEATURED || uGCItemDetails.m_rgchTags.Contains("Featured")))
					{
						SteamCraftInfo steamCraftInfo = new SteamCraftInfo(uGCItemDetails);
						SteamUGC.GetQueryUGCStatistic(results.m_handle, (uint)i, EItemStatistic.k_EItemStatistic_NumFavorites, out steamCraftInfo.totalFavorites);
						SteamUGC.GetQueryUGCStatistic(results.m_handle, (uint)i, EItemStatistic.k_EItemStatistic_NumFollowers, out steamCraftInfo.totalFollowers);
						SteamUGC.GetQueryUGCStatistic(results.m_handle, (uint)i, EItemStatistic.k_EItemStatistic_NumSubscriptions, out steamCraftInfo.totalSubscriptions);
						SteamUGC.GetQueryUGCPreviewURL(results.m_handle, (uint)i, out steamCraftInfo.previewURL, (uint)uGCItemDetails.m_nPreviewFileSize);
						string pchMetadata = "";
						SteamUGC.GetQueryUGCMetadata(results.m_handle, (uint)i, out pchMetadata, 5000u);
						steamCraftInfo.ProcessMetaData(pchMetadata);
						steamCraftInfo.UpdateSteamState();
						CraftEntry item = CraftEntry.Create(null, stock: false, OnEntrySelected, steamItem: true, steamCraftInfo);
						craftList.Add(item);
					}
				}
				int count = craftList.Count;
				for (int j = 0; j < count; j++)
				{
					AddCraftEntryWidget(craftList[j], listContainer);
				}
				UpdateSelectables();
			}
			OnSelectionChanged(null);
			SteamUGC.ReleaseQueryUGCRequest(results.m_handle);
		}

		private void BuildSteamCraftList()
		{
			ClearCraftList();
			craftList = new List<CraftEntry>();
			UpdateSteamListOfCraft(selectedSteamQueryFilter);
			UpdateSteamSubscribedItems();
		}

		private void onButtonSteamSubUnsub()
		{
			if (!(selectedEntry != null))
			{
				return;
			}
			if (SteamItemSubscribed(selectedEntry.GetSteamFileId()))
			{
				steamWorkshopItemUnsubscribe = UIConfirmDialog.Spawn(Localizer.Format("#autoLOC_8002132"), Localizer.Format("#autoLOC_8002133"), Localizer.Format("#autoLOC_226976"), Localizer.Format("#autoLOC_226975"), Localizer.Format("#autoLOC_360842"), delegate(bool b)
				{
					SaveCraftSteamUnsubscribeWarning(b);
					SteamManager.Instance.unsubscribeItem(new PublishedFileId_t(selectedEntry.GetSteamFileId()), onUnsubscribeItemCallback);
				}, delegate
				{
				});
			}
			else
			{
				SteamManager.Instance.subscribeItem(new PublishedFileId_t(selectedEntry.GetSteamFileId()), onSubscribeItemCallback);
			}
		}

		private void onUnsubscribeItemCallback(RemoteStorageUnsubscribePublishedFileResult_t callResult, bool bIOFailure)
		{
			if (!bIOFailure && callResult.m_eResult == EResult.k_EResultOK)
			{
				AnalyticsUtil.LogSteamItemUnsubscribed(AnalyticsUtil.steamItemTypes.craft, callResult.m_nPublishedFileId);
				btnSteamSubUnsubText.text = Localizer.Format("#autoLOC_8002134");
				if (steamTabOpen)
				{
					UpdateSteamListOfCraft(selectedSteamQueryFilter);
				}
				else
				{
					BuildPlayerCraftList();
				}
				UpdateSteamSubscribedItems();
			}
			else
			{
				AnalyticsUtil.LogSteamError(AnalyticsUtil.steamActions.unsubscribe, AnalyticsUtil.steamItemTypes.craft, callResult.m_nPublishedFileId, callResult.m_eResult);
				unableToSubscribeToSteam = PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("SteamError", Localizer.Format("#autoLOC_8002136", SteamManager.Instance.GetUGCFailureReason(callResult.m_eResult)), Localizer.Format("#autoLOC_8002125"), skin, 350f, new DialogGUIButton(Localizer.Format("#autoLOC_226975"), null, dismissOnSelect: true)), persistAcrossScenes: false, skin);
			}
		}

		private void onSubscribeItemCallback(RemoteStorageSubscribePublishedFileResult_t callResult, bool bIOFailure)
		{
			if (!bIOFailure && callResult.m_eResult == EResult.k_EResultOK)
			{
				AnalyticsUtil.LogSteamItemSubscribed(AnalyticsUtil.steamItemTypes.craft, callResult.m_nPublishedFileId);
				btnSteamSubUnsubText.text = Localizer.Format("#autoLOC_8002135");
				if (steamTabOpen)
				{
					UpdateSteamListOfCraft(selectedSteamQueryFilter);
				}
				else
				{
					BuildPlayerCraftList();
				}
				UpdateSteamSubscribedItems();
			}
			else
			{
				AnalyticsUtil.LogSteamError(AnalyticsUtil.steamActions.subscribe, AnalyticsUtil.steamItemTypes.craft, callResult.m_nPublishedFileId, callResult.m_eResult);
				unableToSubscribeToSteam = PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("SteamError", Localizer.Format("#autoLOC_8002137", SteamManager.Instance.GetUGCFailureReason(callResult.m_eResult)), Localizer.Format("#autoLOC_8002125"), skin, 350f, new DialogGUIButton(Localizer.Format("#autoLOC_226975"), null, dismissOnSelect: true)), persistAcrossScenes: false, skin);
			}
		}

		private void SaveCraftSteamUnsubscribeWarning(bool dontShowAgain)
		{
			if (dontShowAgain == GameSettings.CRAFT_STEAM_UNSUBSCRIBE_WARNING)
			{
				GameSettings.CRAFT_STEAM_UNSUBSCRIBE_WARNING = !dontShowAgain;
				GameSettings.SaveGameSettingsOnly();
			}
		}

		private void onSteamFilterOne(bool value)
		{
			if (selectedSteamQueryFilter != 0)
			{
				SteamLoadingText.gameObject.SetActive(value: true);
				ClearCraftList();
				selectedSteamQueryFilter = SteamQueryFilters.VOTE;
				UpdateSteamListOfCraft(selectedSteamQueryFilter);
				UpdateSteamSubscribedItems();
			}
		}

		private void onSteamFilterTwo(bool value)
		{
			if (selectedSteamQueryFilter != SteamQueryFilters.FEATURED)
			{
				SteamLoadingText.gameObject.SetActive(value: true);
				ClearCraftList();
				selectedSteamQueryFilter = SteamQueryFilters.FEATURED;
				UpdateSteamListOfCraft(selectedSteamQueryFilter);
				UpdateSteamSubscribedItems();
			}
		}

		private void onSteamFilterThree(bool value)
		{
			if (selectedSteamQueryFilter != SteamQueryFilters.NEWEST)
			{
				SteamLoadingText.gameObject.SetActive(value: true);
				ClearCraftList();
				selectedSteamQueryFilter = SteamQueryFilters.NEWEST;
				UpdateSteamListOfCraft(selectedSteamQueryFilter);
				UpdateSteamSubscribedItems();
			}
		}

		private void onSteamFilterFour(bool value)
		{
			if (selectedSteamQueryFilter != SteamQueryFilters.SUBSCRIBERS)
			{
				SteamLoadingText.gameObject.SetActive(value: true);
				ClearCraftList();
				selectedSteamQueryFilter = SteamQueryFilters.SUBSCRIBERS;
				UpdateSteamListOfCraft(selectedSteamQueryFilter);
				UpdateSteamSubscribedItems();
			}
		}

		private void OnBtnSteamOverlay()
		{
			string text = "https://steamcommunity.com/workshop/browse/?appid=220200&searchtext=&childpublishedfileid=0&section=readytouseitems&days=-1";
			switch (selectedSteamQueryFilter)
			{
			case SteamQueryFilters.VOTE:
				text += "&section=readytouseitems&days=-1&browsesort=trend&requiredtags%5B%5D=Craft";
				break;
			case SteamQueryFilters.FEATURED:
				text += "&browsesort=trend&requiredtags%5B%5D=Craft&requiredtags%5B%5D=Featured";
				break;
			case SteamQueryFilters.NEWEST:
				text += "&section=readytouseitems&days=-1&browsesort=mostrecent&requiredtags%5B%5D=Craft";
				break;
			case SteamQueryFilters.SUBSCRIBERS:
				text += "&section=readytouseitems&days=-1&browsesort=totaluniquesubscribers&requiredtags%5B%5D=Craft";
				break;
			}
			SteamFriends.ActivateGameOverlayToWebPage(text);
		}

		private void onBtnSteamItemDetails()
		{
			string text = "steam://url/CommunityFilePage/" + selectedEntry.steamCraftInfo.itemDetails.m_nPublishedFileId.m_PublishedFileId;
			Debug.Log("Opening: " + text);
			SteamFriends.ActivateGameOverlayToWebPage(text);
		}

		private void remoteFileSubscribedCallback(RemoteStoragePublishedFileSubscribed_t result, bool bIOFailure)
		{
			if (!bIOFailure)
			{
				BuildPlayerCraftList();
			}
		}

		private void remoteFileUnsubscribedCallback(RemoteStoragePublishedFileUnsubscribed_t result, bool bIOFailure)
		{
			if (!bIOFailure)
			{
				BuildPlayerCraftList();
			}
		}

		private void pipeSelectedItem(CraftEntry sItem, LoadType loadType)
		{
			KSPUpgradePipeline.Process(sItem.configNode, sItem.name, LoadContext.Craft, delegate(ConfigNode n)
			{
				onPipelineFinished(n, sItem, loadType);
			}, delegate(KSPUpgradePipeline.UpgradeFailOption opt, ConfigNode n)
			{
				onPipelineFailed(opt, n, sItem, loadType);
			});
		}

		private void onPipelineFailed(KSPUpgradePipeline.UpgradeFailOption opt, ConfigNode n, CraftEntry sItem, LoadType loadType)
		{
			switch (opt)
			{
			case KSPUpgradePipeline.UpgradeFailOption.LoadAnyway:
				onPipelineFinished(n, sItem, loadType);
				break;
			case KSPUpgradePipeline.UpgradeFailOption.Cancel:
				OnBrowseCancelled();
				break;
			}
		}

		private void onPipelineFinished(ConfigNode n, CraftEntry sItem, LoadType loadType)
		{
			if (n != sItem.configNode)
			{
				sItem.configNode.Save(sItem.fullFilePath + ".original");
				n.Save(sItem.fullFilePath);
			}
			OnFileSelected?.Invoke(sItem.fullFilePath, loadType);
			OnConfigNodeSelected?.Invoke(n, loadType);
		}

		protected void onButtonDelete()
		{
			if (selectedEntry != null)
			{
				PromptDeleteFileConfirm();
			}
		}

		private void PromptDeleteFileConfirm()
		{
			Hide();
			if (window != null)
			{
				window.Dismiss();
			}
			DialogGUIHorizontalLayout dialogGUIHorizontalLayout = new DialogGUIHorizontalLayout();
			dialogGUIHorizontalLayout.AddChild(new DialogGUIFlexibleSpace());
			dialogGUIHorizontalLayout.AddChild(new DialogGUIButton("<color=orange>" + Localizer.Format("#autoLOC_129950") + "</color>", delegate
			{
				OnDeleteConfirm();
				ClearSelection();
				Show();
			}, 80f, 30f, true));
			dialogGUIHorizontalLayout.AddChild(new DialogGUIButton(Localizer.Format("#autoLOC_129951"), Show, 80f, 30f, true));
			DialogGUIVerticalLayout dialogGUIVerticalLayout = new DialogGUIVerticalLayout();
			dialogGUIVerticalLayout.AddChild(new DialogGUILabel(Localizer.Format("#autoLOC_7003238"), expandW: true));
			dialogGUIVerticalLayout.AddChild(dialogGUIHorizontalLayout);
			window = PopupDialog.SpawnPopupDialog(new MultiOptionDialog("DeleteFileConfirmation", "", Localizer.Format("#autoLOC_7003239"), skin, dialogGUIVerticalLayout), persistAcrossScenes: false, skin);
			window.OnDismiss = onButtonCancel;
		}

		private void OnDeleteConfirm()
		{
			string path = KSPUtil.ApplicationRootPath + selectedEntry.thumbURL + ".png";
			if (File.Exists(path))
			{
				File.Delete(path);
			}
			string path2 = selectedEntry.fullFilePath.Replace(".craft", ".loadmeta");
			if (File.Exists(path2))
			{
				File.Delete(path2);
			}
			string fullFilePath = selectedEntry.fullFilePath;
			File.Delete(fullFilePath);
			directoryController.UpdateDirectoryDisplay(fullFilePath);
			BuildPlayerCraftList();
		}

		protected void onVABtabToggle(bool isSelected)
		{
			if (!isSelected)
			{
				return;
			}
			steamTabOpen = false;
			SteamLoadingText.gameObject.SetActive(value: false);
			SteamFilterHolder.SetActive(value: false);
			scrollView.offsetMax = new Vector2(scrollView.offsetMax.x, -56f);
			facility = EditorFacility.VAB;
			if (SteamManager.Initialized)
			{
				if (SteamDetailsPanel.gameObject.activeInHierarchy)
				{
					SteamDetailsPanel.Transition(0);
				}
				SteamManager.Instance.RegisterRemoteSubUnsubEvents(remoteFileSubscribedCallback, remoteFileUnsubscribedCallback);
			}
			craftSubfolder = ShipConstruction.GetShipsSubfolderFor(facility);
			if (directoryController != null)
			{
				directoryController.ShowDirectoryTreeForEditor(facility);
				directoryController.CraftListUpdate();
			}
		}

		protected void onSPHtabToggle(bool isSelected)
		{
			if (isSelected)
			{
				steamTabOpen = false;
				SteamLoadingText.gameObject.SetActive(value: false);
				SteamFilterHolder.SetActive(value: false);
				scrollView.offsetMax = new Vector2(scrollView.offsetMax.x, -56f);
				facility = EditorFacility.SPH;
				if (SteamManager.Initialized)
				{
					SteamDetailsPanel.Transition(0);
					SteamManager.Instance.RegisterRemoteSubUnsubEvents(remoteFileSubscribedCallback, remoteFileUnsubscribedCallback);
				}
				craftSubfolder = ShipConstruction.GetShipsSubfolderFor(facility);
				if (directoryController != null)
				{
					directoryController.ShowDirectoryTreeForEditor(facility);
					directoryController.CraftListUpdate();
				}
			}
		}

		protected void onSteamtabToggle(bool isSelected)
		{
			if (isSelected)
			{
				steamTabOpen = true;
				SteamLoadingText.gameObject.SetActive(value: true);
				SteamDetailsPanel.Transition(1);
				scrollView.offsetMax = new Vector2(scrollView.offsetMax.x, -82f);
				SteamFilterHolder.SetActive(value: true);
				BuildSteamCraftList();
				SteamManager.Instance.RemoveRemoteSubUnsubEvents();
			}
		}

		protected void setbottomButtons(CraftEntry selectedEntry, bool steamMode)
		{
			btnDelete.gameObject.SetActive(!steamMode && !DirectoryController.IsTrainingScenario && selectedEntry != null && !selectedEntry.isStock && !selectedEntry.steamItem);
			btnLoad.gameObject.SetActive(!steamMode);
			btnMerge.gameObject.SetActive(!steamMode && !DirectoryController.IsTrainingScenario);
			SteamButtons.SetActive(steamMode && !DirectoryController.IsTrainingScenario);
			UpdateExplicitNavigation();
		}
	}
}

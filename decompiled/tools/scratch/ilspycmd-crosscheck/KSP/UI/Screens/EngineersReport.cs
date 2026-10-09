using System.Collections;
using System.Collections.Generic;
using Highlighting;
using KSP.Localization;
using PreFlightTests;
using RUI.Algorithms;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public class EngineersReport : UIApp
	{
		private class TestWrapper
		{
			private static int idCounter;

			private int id;

			private List<Part> selectors;

			public IDesignConcern test { get; private set; }

			public UIListItem container { get; private set; }

			public UIStateImage containerStateImage { get; private set; }

			public TestWrapper(IDesignConcern test)
			{
				id = ++idCounter;
				this.test = test;
				container = Instance.cascadingListCheck.CreateBodyCollapsable(Instance.listItem_BodySeverity_prefab, GetFormattedTestTitle(), GetFormattedTestDescription());
				containerStateImage = container.GetComponentInChildren<UIStateImage>();
				containerStateImage.SetState(test.GetSeverity().ToString().ToLower());
				container.transform.SetParent(Instance.cascadingListCheck.transform, worldPositionStays: false);
				PointerEnterExitHandler component = container.GetComponent<PointerEnterExitHandler>();
				component.onPointerEnter.AddListener(MouseInput_PointerEnter);
				component.onPointerExit.AddListener(MouseInput_PointerExit);
				selectors = new List<Part>();
			}

			public void UpdateContainer()
			{
				Instance.cascadingListCheck.UpdateBodyCollapsable(container, GetFormattedTestTitle(), GetFormattedTestDescription());
				UpdateStateImage();
			}

			private string GetFormattedTestTitle()
			{
				return "     [+] " + test.GetConcernTitle();
			}

			private string GetFormattedTestDescription()
			{
				return "     [-] " + test.GetConcernDescription();
			}

			private void UpdateStateImage()
			{
				if (containerStateImage != null)
				{
					containerStateImage.SetState(test.GetSeverity().ToString().ToLower());
				}
			}

			public bool RunTest()
			{
				return test.Test();
			}

			protected void MouseInput_PointerEnter(PointerEventData eventData)
			{
				if (test.GetPreviousResult())
				{
					return;
				}
				Instance.scheduler.Schedule(id, delegate
				{
					List<Part> affectedParts = test.GetAffectedParts();
					if (affectedParts != null)
					{
						int i = 0;
						for (int count = affectedParts.Count; i < count; i++)
						{
							Part part = affectedParts[i];
							part.Highlight(Highlighter.colorPartEngineerAppHighlight);
							selectors.Add(part);
						}
					}
				});
			}

			protected void MouseInput_PointerExit(PointerEventData eventData)
			{
				Instance.scheduler.Deschedule(id, delegate
				{
					int i = 0;
					for (int count = selectors.Count; i < count; i++)
					{
						selectors[i].Highlight(Part.defaultHighlightNone);
					}
					selectors.Clear();
				});
			}
		}

		public static EngineersReport Instance;

		public static bool Ready;

		[SerializeField]
		private GameObject appFramePrefab;

		private GenericAppFrame appFrame;

		[SerializeField]
		private GameObject cascadingListPrefab;

		private GenericCascadingList cascadingListInfo;

		private GenericCascadingList cascadingListCheck;

		[SerializeField]
		private UIListItem listItem_SeveritySelector_prefab;

		private UIListItem listItem_SeveritySelector;

		[SerializeField]
		private UIListItem listItem_BodySeverity_prefab;

		private UIListItem listItem_partCountZero;

		private UIListItem listItem_allTestsPassed;

		private TextMeshProUGUI partCountLH;

		private TextMeshProUGUI partCountRH;

		private TextMeshProUGUI partMassLH;

		private TextMeshProUGUI partMassRH;

		private TextMeshProUGUI sizeLH;

		private TextMeshProUGUI sizeRH;

		private float partCount;

		private float partLimit;

		private float totalMass;

		private float massLimit;

		private Vector3 craftSize;

		private Vector3 maxSize;

		private SpaceCenterFacility editorFacility;

		private SpaceCenterFacility launchFacility;

		private bool allGood;

		private UICascadingList.CascadingListItem designConcernCascadingItem;

		private RUIHoverController scheduler;

		private List<TestWrapper> tests = new List<TestWrapper>();

		private UIListItem designConcernHeader;

		private Coroutine updateRoutine;

		private Coroutine testRoutine;

		public static SCCFlowGraphUCFinder sccFlowGraphUCFinder;

		private List<TestWrapper> tempBodyList = new List<TestWrapper>();

		private UIRadioButton btnNotice;

		private UIRadioButton btnWarning;

		private UIRadioButton btnCritical;

		private bool severityNotice = true;

		private bool severityWarning = true;

		private bool severityCritical = true;

		private static string cacheAutoLOC_442833;

		private static string cacheAutoLOC_443059;

		private static string cacheAutoLOC_443064;

		private static string cacheAutoLOC_443343;

		private static string cacheAutoLOC_443417;

		private static string cacheAutoLOC_443418;

		private static string cacheAutoLOC_443419;

		private static string cacheAutoLOC_443420;

		private static string cacheAutoLOC_7001411;

		private static string cacheAutoLOC_442811;

		private void Start()
		{
			EditorFacility editorFacility = EditorDriver.editorFacility;
			if (editorFacility != EditorFacility.VAB && editorFacility == EditorFacility.SPH)
			{
				this.editorFacility = SpaceCenterFacility.SpaceplaneHangar;
				launchFacility = SpaceCenterFacility.Runway;
			}
			else
			{
				this.editorFacility = SpaceCenterFacility.VehicleAssemblyBuilding;
				launchFacility = SpaceCenterFacility.LaunchPad;
			}
		}

		protected override bool OnAppAboutToStart()
		{
			return HighLogic.LoadedSceneIsEditor;
		}

		protected override ApplicationLauncher.AppScenes GetAppScenes()
		{
			return ApplicationLauncher.AppScenes.VAB | ApplicationLauncher.AppScenes.SPH;
		}

		protected override Vector3 GetAppScreenPos(Vector3 defaultAnchorPos)
		{
			return defaultAnchorPos;
		}

		protected override void OnAppInitialized()
		{
			Instance = this;
			GameEvents.onEditorShipModified.Add(OnCraftModified);
			GameEvents.onEditorShipCrewModified.Add(OnCrewModified);
			GameEvents.onEditorShowPartList.Add(OnCraftModified);
			GameEvents.onEditorStarted.Add(OnCraftModified_delayed);
			GameEvents.StageManager.OnGUIStageSequenceModified.Add(OnCraftModified);
			GameEvents.StageManager.OnGUIStageAdded.Add(OnStageAddedDeleted);
			GameEvents.StageManager.OnGUIStageRemoved.Add(OnStageAddedDeleted);
			BaseCrewAssignmentDialog.onCrewDialogChange.Add(onCrewDialogChange);
			appFrame = Object.Instantiate(appFramePrefab).GetComponent<GenericAppFrame>();
			appFrame.Setup(base.appLauncherButton, base.name, cacheAutoLOC_442811);
			appFrame.AddGlobalInputDelegate(base.MouseInput_PointerEnter, base.MouseInput_PointerExit);
			ApplicationLauncher.Instance.AddOnRepositionCallback(appFrame.Reposition);
			HideApp();
			cascadingListInfo = Object.Instantiate(cascadingListPrefab).GetComponent<GenericCascadingList>();
			cascadingListInfo.Setup(appFrame.scrollList);
			cascadingListInfo.transform.SetParent(ApplicationLauncher.Instance.appSpace, worldPositionStays: false);
			cascadingListCheck = Object.Instantiate(cascadingListPrefab).GetComponent<GenericCascadingList>();
			cascadingListCheck.Setup(appFrame.scrollList);
			cascadingListCheck.transform.SetParent(ApplicationLauncher.Instance.appSpace, worldPositionStays: false);
			cascadingListCheck.ruiList.DeleteBodyOnUpdate = false;
			cascadingListCheck.ruiList.DeleteHeaderOnUpdate = false;
			CreateCraftStats();
			OnCraftModified_delayed();
			designConcernHeader = cascadingListCheck.CreateHeader(cacheAutoLOC_442833, out var button, scaleBg: true);
			designConcernCascadingItem = cascadingListCheck.ruiList.AddCascadingItem(designConcernHeader, cascadingListCheck.CreateFooter(), new List<UIListItem>(), button);
			CreateStockDesignConcern();
			updateRoutine = StartCoroutine(RunTests());
			scheduler = new RUIHoverController(this);
			Ready = true;
			GameEvents.onGUIEngineersReportReady.Fire();
		}

		protected override void OnAppDestroy()
		{
			GameEvents.onEditorShipModified.Remove(OnCraftModified);
			GameEvents.onEditorShipCrewModified.Remove(OnCrewModified);
			GameEvents.onEditorShowPartList.Remove(OnCraftModified);
			GameEvents.onEditorStarted.Remove(OnCraftModified_delayed);
			GameEvents.StageManager.OnGUIStageSequenceModified.Remove(OnCraftModified);
			GameEvents.StageManager.OnGUIStageAdded.Remove(OnStageAddedDeleted);
			GameEvents.StageManager.OnGUIStageRemoved.Remove(OnStageAddedDeleted);
			BaseCrewAssignmentDialog.onCrewDialogChange.Remove(onCrewDialogChange);
			if (appFrame != null)
			{
				if ((bool)ApplicationLauncher.Instance)
				{
					ApplicationLauncher.Instance.RemoveOnRepositionCallback(appFrame.Reposition);
				}
				appFrame.gameObject.DestroyGameObject();
			}
			if (cascadingListInfo != null)
			{
				cascadingListInfo.gameObject.DestroyGameObject();
			}
			if (cascadingListCheck != null)
			{
				cascadingListCheck.gameObject.DestroyGameObject();
			}
			Ready = false;
			GameEvents.onGUIEngineersReportDestroy.Fire();
			if (Instance != null && Instance == this)
			{
				Instance = null;
			}
		}

		protected override void DisplayApp()
		{
			if (appFrame != null)
			{
				appFrame.gameObject.SetActive(value: true);
				UpdateCratStats(EditorLogic.fetch.ship);
				UpdateAndReposition();
			}
		}

		protected override void HideApp()
		{
			if (appFrame != null)
			{
				appFrame.gameObject.SetActive(value: false);
			}
		}

		private void OnStageAddedDeleted(int inverseStageIndex)
		{
			OnCraftModified(EditorLogic.fetch.ship);
		}

		private void onCrewDialogChange(VesselCrewManifest vcm)
		{
			OnCraftModified();
		}

		private void OnCraftModified()
		{
			OnCraftModified(EditorLogic.fetch.ship);
		}

		private void OnCraftModified_delayed()
		{
			Start();
			StartCoroutine(CallbackUtil.DelayedCallback(3, delegate
			{
				OnCraftModified(EditorLogic.fetch.ship);
			}));
		}

		private void OnCraftModified(ShipConstruct ship)
		{
			if (updateRoutine != null)
			{
				StopCoroutine(updateRoutine);
			}
			updateRoutine = StartCoroutine(UpdateCraftStatsRoutine(ship));
			if (testRoutine != null)
			{
				StopCoroutine(testRoutine);
			}
			testRoutine = StartCoroutine(RunTests());
		}

		private void OnCrewModified(VesselCrewManifest vcm)
		{
			OnCraftModified();
		}

		public void AddTest(IDesignConcern test)
		{
			if (ShouldTest(test))
			{
				TestWrapper item = new TestWrapper(test);
				tests.Add(item);
			}
		}

		public void RemoveTest(IDesignConcern test)
		{
			TestWrapper item = tests.Find((TestWrapper a) => a.test.Equals(test));
			if (tests.Contains(item))
			{
				tests.Remove(item);
			}
			else
			{
				Debug.LogError("[Engineer's Report] couldn't find the test.");
			}
		}

		private IEnumerator RunTests()
		{
			yield return null;
			yield return null;
			sccFlowGraphUCFinder = new SCCFlowGraphUCFinder(EditorLogic.fetch.ship.Parts);
			if (EditorLogic.fetch.ship.parts.Count != 0)
			{
				int i = 0;
				for (int count = tests.Count; i < count; i++)
				{
					tests[i].RunTest();
				}
			}
			UpdateDesignConcern();
		}

		private void UpdateDesignConcern()
		{
			cascadingListCheck.ruiList.UpdateCascadingItem(ref designConcernCascadingItem, designConcernHeader, cascadingListCheck.CreateFooter(), UpdateDesignConcernBody, null);
			int i = 0;
			for (int count = tempBodyList.Count; i < count; i++)
			{
				tempBodyList[i].UpdateContainer();
			}
		}

		private List<UIListItem> UpdateDesignConcernBody()
		{
			tempBodyList.Clear();
			bool flag = false;
			List<UIListItem> list = new List<UIListItem>();
			list.Add(listItem_SeveritySelector);
			if (EditorLogic.fetch.ship.parts.Count != 0)
			{
				int i = 0;
				for (int count = tests.Count; i < count; i++)
				{
					if (!tests[i].test.GetPreviousResult() && ShowSeverity(tests[i]))
					{
						list.Add(tests[i].container);
						tempBodyList.Add(tests[i]);
						flag = true;
					}
				}
				if (!flag)
				{
					list.Add(listItem_allTestsPassed);
				}
			}
			else
			{
				list.Add(listItem_partCountZero);
			}
			return list;
		}

		private void CreateStockDesignConcern()
		{
			listItem_SeveritySelector = CreateSeveritySelector();
			listItem_SeveritySelector.transform.SetParent(cascadingListCheck.transform, worldPositionStays: false);
			listItem_partCountZero = cascadingListCheck.CreateBody(cacheAutoLOC_443059);
			listItem_partCountZero.transform.SetParent(cascadingListCheck.transform, worldPositionStays: false);
			listItem_allTestsPassed = cascadingListCheck.CreateBody(cacheAutoLOC_443064);
			listItem_allTestsPassed.transform.SetParent(cascadingListCheck.transform, worldPositionStays: false);
			AddTest(new RequiredIScienceDataTransmitter());
			AddTest(new ParachuteOnFirstStage());
			AddTest(new ParachuteOnEngineStage());
			AddTest(new ParachutePresent());
			AddTest(new StationHubAttachments());
			AddTest(new NonRootCmdMissaligned());
			AddTest(new DecouplerFacing());
			AddTest(new MissingCrew());
			AddTest(new DecouplersBeforeClamps());
			AddTest(new EnginesJettisonedBeforeUse());
			AddTest(new LadderPresent());
			AddTest(new DockingPortFacing());
			AddTest(new DockingPortAsDecoupler());
			AddTest(new HatchObstructed());
			AddTest(new LandingGearPresent(EditorDriver.editorFacility));
			AddTest(new NoControlSources());
			AddTest(new KerbalSeatCollision());
			AddTest(new ElectricConsumerAndNoCharge());
			AddTest(new ElectricChargeAndNoConsumer());
			AddTest(new ElectricChargeAndNoBattery());
			AddTest(new ElectricBatteryAndNoCharge());
			AddTest(new AntennaPresent());
			IEnumerator<PartResourceDefinition> enumerator = PartResourceLibrary.Instance.resourceDefinitions.GetEnumerator();
			while (enumerator.MoveNext())
			{
				PartResourceDefinition current = enumerator.Current;
				if (current.resourceFlowMode != 0 && current.name != "ElectricCharge")
				{
					AddTest(new ResourceContainersReachable(current));
					AddTest(new ResourceConsumersReachable(current));
				}
			}
		}

		private bool ShowSeverity(TestWrapper testWrapper)
		{
			if (testWrapper.test.GetSeverity() == DesignConcernSeverity.NOTICE && severityNotice)
			{
				return true;
			}
			if (testWrapper.test.GetSeverity() == DesignConcernSeverity.WARNING && severityWarning)
			{
				return true;
			}
			if (testWrapper.test.GetSeverity() == DesignConcernSeverity.CRITICAL && severityCritical)
			{
				return true;
			}
			return false;
		}

		private UIListItem CreateSeveritySelector()
		{
			UIListItem uIListItem = Object.Instantiate(listItem_SeveritySelector_prefab);
			UIRadioButton[] componentsInChildren = uIListItem.GetComponentsInChildren<UIRadioButton>();
			int num = componentsInChildren.Length;
			for (int i = 0; i < num; i++)
			{
				UIRadioButton uIRadioButton = componentsInChildren[i];
				switch (uIRadioButton.gameObject.name)
				{
				case "radioBtn_critical":
					btnCritical = uIRadioButton;
					break;
				case "radioBtn_warning":
					btnWarning = uIRadioButton;
					break;
				case "radioBtn_notice":
					btnNotice = uIRadioButton;
					break;
				}
			}
			btnNotice.onTrue.AddListener(Notice_OnTrue);
			btnNotice.onFalse.AddListener(Notice_OnFalse);
			btnWarning.onTrue.AddListener(Warning_OnTrue);
			btnWarning.onFalse.AddListener(Warning_OnFalse);
			btnCritical.onTrue.AddListener(Critical_OnTrue);
			btnCritical.onFalse.AddListener(Critical_OnFalse);
			return uIListItem;
		}

		private void Notice_OnTrue(PointerEventData data, UIRadioButton.CallType callType)
		{
			severityNotice = true;
			UpdateAndReposition();
		}

		private void Notice_OnFalse(PointerEventData data, UIRadioButton.CallType callType)
		{
			severityNotice = false;
			UpdateAndReposition();
		}

		private void Warning_OnTrue(PointerEventData data, UIRadioButton.CallType callType)
		{
			severityWarning = true;
			UpdateAndReposition();
		}

		private void Warning_OnFalse(PointerEventData data, UIRadioButton.CallType callType)
		{
			severityWarning = false;
			UpdateAndReposition();
		}

		private void Critical_OnTrue(PointerEventData data, UIRadioButton.CallType callType)
		{
			severityCritical = true;
			UpdateAndReposition();
		}

		private void Critical_OnFalse(PointerEventData data, UIRadioButton.CallType callType)
		{
			severityCritical = false;
			UpdateAndReposition();
		}

		private void UpdateAndReposition()
		{
			UpdateDesignConcern();
		}

		private UICascadingList.CascadingListItem CreateCraftStats()
		{
			Button button;
			UIListItem header = cascadingListInfo.CreateHeader(cacheAutoLOC_443343, out button, scaleBg: true);
			return cascadingListInfo.ruiList.AddCascadingItem(header, cascadingListInfo.CreateFooter(), CreateCraftStatsbody(), button);
		}

		private List<UIListItem> CreateCraftStatsbody()
		{
			List<UIListItem> list = new List<UIListItem>();
			UIListItem uIListItem = cascadingListInfo.CreateBodyKeyValueAutofit("Parts:", "");
			partCountLH = uIListItem.GetTextElement("keyRich");
			partCountRH = uIListItem.GetTextElement("valueRich");
			list.Add(uIListItem);
			uIListItem = cascadingListInfo.CreateBodyKeyValueAutofit("Mass:", "");
			partMassLH = uIListItem.GetTextElement("keyRich");
			partMassRH = uIListItem.GetTextElement("valueRich");
			list.Add(uIListItem);
			uIListItem = cascadingListInfo.CreateBodyKeyValueAutofit("Size\nHeight:\nWidth:\nLength:", "Size\nHeight:\nWidth:\nLength:");
			sizeLH = uIListItem.GetTextElement("keyRich");
			sizeRH = uIListItem.GetTextElement("valueRich");
			list.Add(uIListItem);
			return list;
		}

		private IEnumerator UpdateCraftStatsRoutine(ShipConstruct ship)
		{
			yield return new WaitForEndOfFrame();
			UpdateCratStats(ship);
		}

		private void UpdateCratStats(ShipConstruct ship)
		{
			partCount = ship.parts.Count;
			partLimit = GameVariables.Instance.GetPartCountLimit(ScenarioUpgradeableFacilities.GetFacilityLevel(editorFacility), editorFacility == SpaceCenterFacility.VehicleAssemblyBuilding);
			totalMass = ship.GetTotalMass();
			massLimit = GameVariables.Instance.GetCraftMassLimit(ScenarioUpgradeableFacilities.GetFacilityLevel(launchFacility), launchFacility == SpaceCenterFacility.LaunchPad);
			craftSize = ShipConstruction.CalculateCraftSize(ship);
			maxSize = GameVariables.Instance.GetCraftSizeLimit(ScenarioUpgradeableFacilities.GetFacilityLevel(launchFacility), launchFacility == SpaceCenterFacility.LaunchPad);
			string kSPNeutralUIGrey = XKCDColors.HexFormat.KSPNeutralUIGrey;
			string text = ((partCount <= partLimit) ? XKCDColors.HexFormat.KSPBadassGreen : XKCDColors.HexFormat.KSPNotSoGoodOrange);
			partCountLH.text = Localizer.Format("#autoLOC_443389", kSPNeutralUIGrey);
			if (partLimit < 2.1474836E+09f)
			{
				partCountRH.text = "<color=" + text + ">" + partCount.ToString("0") + " / " + partLimit.ToString("0") + "</color>";
			}
			else
			{
				partCountRH.text = "<color=" + text + ">" + partCount.ToString("0") + "</color>";
			}
			string text2 = ((totalMass <= massLimit) ? XKCDColors.HexFormat.KSPBadassGreen : XKCDColors.HexFormat.KSPNotSoGoodOrange);
			partMassLH.text = Localizer.Format("#autoLOC_443401", kSPNeutralUIGrey);
			if (massLimit < float.MaxValue)
			{
				partMassRH.text = Localizer.Format("#autoLOC_443405", text2, totalMass.ToString("N3"), massLimit.ToString("N1"));
			}
			else
			{
				partMassRH.text = Localizer.Format("#autoLOC_443409", text2, totalMass.ToString("N3"));
			}
			string text3 = ((craftSize.y <= maxSize.y) ? XKCDColors.HexFormat.KSPBadassGreen : XKCDColors.HexFormat.KSPNotSoGoodOrange);
			string text4 = ((craftSize.x <= maxSize.x) ? XKCDColors.HexFormat.KSPBadassGreen : XKCDColors.HexFormat.KSPNotSoGoodOrange);
			string text5 = ((craftSize.z <= maxSize.z) ? XKCDColors.HexFormat.KSPBadassGreen : XKCDColors.HexFormat.KSPNotSoGoodOrange);
			sizeLH.text = "<line-height=110%><color=" + kSPNeutralUIGrey + ">" + cacheAutoLOC_443417 + "</color>\n<color=" + kSPNeutralUIGrey + ">" + cacheAutoLOC_443418 + "</color>\n<color=" + kSPNeutralUIGrey + ">" + cacheAutoLOC_443419 + "</color>\n<color=" + kSPNeutralUIGrey + ">" + cacheAutoLOC_443420 + "</color></line-height>";
			string text6 = cacheAutoLOC_7001411;
			if (maxSize.x < float.MaxValue && maxSize.y < float.MaxValue && maxSize.z < float.MaxValue)
			{
				sizeRH.text = "<line-height=110%>  \n<color=" + text3 + ">" + KSPUtil.LocalizeNumber(craftSize.y, "0.0") + text6 + " / " + KSPUtil.LocalizeNumber(maxSize.y, "0.0") + text6 + "</color>\n<color=" + text4 + ">" + KSPUtil.LocalizeNumber(craftSize.x, "0.0") + text6 + " / " + KSPUtil.LocalizeNumber(maxSize.x, "0.0") + text6 + "</color>\n<color=" + text5 + ">" + KSPUtil.LocalizeNumber(craftSize.z, "0.0") + text6 + " / " + KSPUtil.LocalizeNumber(maxSize.z, "0.0") + text6 + "</color></line-height>";
			}
			else
			{
				sizeRH.text = "<line-height=110%> \n<color=" + text3 + ">" + KSPUtil.LocalizeNumber(craftSize.y, "0.0") + text6 + "</color>\n<color=" + text4 + ">" + KSPUtil.LocalizeNumber(craftSize.x, "0.0") + text6 + "</color>\n<color=" + text5 + ">" + KSPUtil.LocalizeNumber(craftSize.z, "0.0") + text6 + "</color></line-height>";
			}
			allGood = partCount <= partLimit && totalMass <= massLimit && craftSize.x <= maxSize.x && craftSize.y <= maxSize.y && craftSize.z <= maxSize.z;
			appFrame.header.color = (allGood ? XKCDColors.ElectricLime : XKCDColors.Orange);
			if (!allGood)
			{
				base.appLauncherButton.sprite.color = XKCDColors.Orange;
			}
			if (allGood)
			{
				base.appLauncherButton.sprite.color = Color.white;
			}
		}

		public bool ShouldTest(IDesignConcern test)
		{
			if (EditorDriver.editorFacility == EditorFacility.SPH && (test.GetEditorFacilities() & EditorFacilities.SPH) != 0)
			{
				return true;
			}
			if (EditorDriver.editorFacility == EditorFacility.VAB)
			{
				return (test.GetEditorFacilities() & EditorFacilities.VAB) != 0;
			}
			return false;
		}

		internal static void CacheLocalStrings()
		{
			cacheAutoLOC_442833 = Localizer.Format("#autoLOC_442833");
			cacheAutoLOC_443059 = Localizer.Format("#autoLOC_443059");
			cacheAutoLOC_443064 = Localizer.Format("#autoLOC_443064");
			cacheAutoLOC_443343 = "<color=#e6752a>" + Localizer.Format("#autoLOC_443343") + "</color>";
			cacheAutoLOC_443417 = Localizer.Format("#autoLOC_443417");
			cacheAutoLOC_443418 = Localizer.Format("#autoLOC_443418");
			cacheAutoLOC_443419 = Localizer.Format("#autoLOC_443419");
			cacheAutoLOC_443420 = Localizer.Format("#autoLOC_443420");
			cacheAutoLOC_7001411 = Localizer.Format("#autoLOC_7001411");
			cacheAutoLOC_442811 = Localizer.Format("#autoLOC_442811");
		}
	}
}

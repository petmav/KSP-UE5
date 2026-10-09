using System.Collections;
using System.Collections.Generic;
using Expansions;
using Expansions.Serenity;
using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.Events;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public class EditorActionGroups : MonoBehaviour, IPointerEnterHandler, IEventSystemHandler, IPointerExitHandler
	{
		private enum ColumnDirection
		{
			Left,
			Right
		}

		public int currentSelectedIndex;

		public bool interfaceActive;

		public UISelectableGridLayoutGroup actionGroupList;

		public UISelectableGridLayoutGroup groupActionsList;

		public UISelectableGridLayoutGroup partActionList;

		public TextMeshProUGUI GroupActionsHeading;

		public TextMeshProUGUI actionGroupColumnHeaderPrefab;

		private TextMeshProUGUI axisGroupHeader;

		private TextMeshProUGUI controllersHeader;

		public EditorActionOverrideGroup actionOverrideGroupHeaderPrefab;

		public EditorActionOverrideGroup separatorGroupHeaderPrefab;

		public EditorActionOverrideToggle actionOverrideTogglePrefab;

		public EditorActionGroup actionGroupPrefab;

		public EditorActionController actionControllerPrefab;

		public EditorActionPartItem groupPartTitlePrefab;

		public GameObject groupPartActionPrefab;

		public EditorActionPartItem partActionTitlePrefab;

		public EditorActionPartItem partActionTextPrefab;

		public EditorActionPartItem partActionPrefab;

		public EditorActionPartReset partActionResetPrefab;

		public EditorActionControllerHeader actionControllerHeaderPrefab;

		public EditorActionControllerOpenButton actionControllerOpenButtonPrefab;

		public Toggle additionalActionsToggle;

		public bool isMouseOver;

		private bool lockGroupOverride;

		private int[] overrideGroupIndices;

		private bool[] overrideGroupOpen;

		private int selectedGroupOverride;

		private uint selectedControllerId;

		private ModuleRoboticController selectedController;

		private int selectedGroup = 1;

		private EditorActionGroupType selectedGroupType;

		private MenuNavigation menuNavigation;

		private List<Selectable> mainSelectables = new List<Selectable>();

		private List<Selectable> actiongroupSelectables = new List<Selectable>();

		private List<Selectable> groupActionsSelectables = new List<Selectable>();

		private List<Selectable> selectionSelectables = new List<Selectable>();

		private Button flightExitButton;

		private Selectable currentSelectable;

		private GameObject cachedCurrentSelected;

		private bool currentColumnFound;

		private int lastSelectedItemColumn;

		private int lastSelectedItemIndex;

		private List<EditorActionPartSelector> selectedParts = new List<EditorActionPartSelector>();

		private List<ModuleRoboticController> cacheControllers;

		public static EditorActionGroups Instance { get; private set; }

		private bool[] overrideDefault
		{
			get
			{
				if (HighLogic.LoadedSceneIsFlight)
				{
					return FlightGlobals.fetch.activeVessel.OverrideDefault;
				}
				return EditorLogic.fetch.ship.OverrideDefault;
			}
		}

		private KSPActionGroup[] overrideActionControl
		{
			get
			{
				if (HighLogic.LoadedSceneIsFlight)
				{
					return FlightGlobals.fetch.activeVessel.OverrideActionControl;
				}
				return EditorLogic.fetch.ship.OverrideActionControl;
			}
		}

		private KSPAxisGroup[] overrideAxisControl
		{
			get
			{
				if (HighLogic.LoadedSceneIsFlight)
				{
					return FlightGlobals.fetch.activeVessel.OverrideAxisControl;
				}
				return EditorLogic.fetch.ship.OverrideAxisControl;
			}
		}

		private bool OverrideDefault
		{
			get
			{
				if (selectedGroupOverride > 0)
				{
					return overrideDefault[selectedGroupOverride - 1];
				}
				return false;
			}
		}

		public KSPActionGroup SelectedGroup
		{
			get
			{
				if (selectedGroupType == EditorActionGroupType.Action)
				{
					return (KSPActionGroup)selectedGroup;
				}
				return KSPActionGroup.None;
			}
		}

		public KSPAxisGroup SelectedAxis
		{
			get
			{
				if (selectedGroupType == EditorActionGroupType.Axis)
				{
					return (KSPAxisGroup)selectedGroup;
				}
				return KSPAxisGroup.None;
			}
		}

		private void Awake()
		{
			Instance = this;
			axisGroupHeader = null;
			GameEvents.onEditorUndo.Add(OnEditorUndoRedo);
			GameEvents.onEditorRedo.Add(OnEditorUndoRedo);
			GameEvents.onRoboticControllerAxesChanged.Add(OnControllerAxesOrActionsChanged);
			GameEvents.onRoboticControllerActionsChanged.Add(OnControllerAxesOrActionsChanged);
		}

		private void Start()
		{
			actionGroupList.onSelectItem = SelectGroup;
			additionalActionsToggle.isOn = GameSettings.ADDITIONAL_ACTION_GROUPS;
			additionalActionsToggle.onValueChanged.AddListener(ToggleAdditionalActions);
			ClearSelection(reconstruct: false);
			ConstructGroupList();
			if (HighLogic.LoadedSceneIsEditor || HighLogic.LoadedSceneIsFlight)
			{
				InitializeMenuNavigation();
			}
		}

		private void Update()
		{
			if (Input.GetKeyDown(KeyCode.LeftArrow))
			{
				NavigateColumn(ColumnDirection.Left);
				CacheCurrentSelectable();
			}
			if (Input.GetKeyDown(KeyCode.RightArrow))
			{
				NavigateColumn(ColumnDirection.Right);
				CacheCurrentSelectable();
			}
			if (Input.GetKeyDown(KeyCode.UpArrow) || Input.GetKeyDown(KeyCode.DownArrow))
			{
				CacheCurrentSelectable();
			}
			if ((Input.GetKeyUp(KeyCode.Return) || Input.GetKeyUp(KeyCode.KeypadEnter)) && menuNavigation != null && menuNavigation.isActiveAndEnabled && EventSystem.current != null && EventSystem.current.currentSelectedGameObject == null)
			{
				switch (lastSelectedItemColumn)
				{
				case 1:
					if (actiongroupSelectables.Count > 0)
					{
						SelectNextColumnItem(actiongroupSelectables, Mathf.Clamp(lastSelectedItemIndex, 0, actiongroupSelectables.Count));
					}
					break;
				case 2:
					if (groupActionsSelectables.Count > 0)
					{
						SelectNextColumnItem(groupActionsSelectables, Mathf.Clamp(lastSelectedItemIndex, 0, groupActionsSelectables.Count));
					}
					break;
				case 3:
					if (selectionSelectables.Count > 0)
					{
						SelectNextColumnItem(selectionSelectables, Mathf.Clamp(lastSelectedItemIndex, 0, selectionSelectables.Count));
					}
					break;
				}
			}
			if (interfaceActive && InputLockManager.GetControlLock("ActionGroupPanelOpen") == ControlTypes.None)
			{
				InputLockManager.SetControlLock(ControlTypes.CAMERAMODES, "ActionGroupPanelOpen");
			}
			else if (!interfaceActive && InputLockManager.GetControlLock("ActionGroupPanelOpen") != ControlTypes.None)
			{
				InputLockManager.RemoveControlLock("ActionGroupPanelOpen");
			}
		}

		private void OnDestroy()
		{
			GameEvents.onEditorUndo.Remove(OnEditorUndoRedo);
			GameEvents.onEditorRedo.Remove(OnEditorUndoRedo);
			GameEvents.onRoboticControllerAxesChanged.Remove(OnControllerAxesOrActionsChanged);
			GameEvents.onRoboticControllerActionsChanged.Remove(OnControllerAxesOrActionsChanged);
			if (EditorLogic.fetch != null && EditorLogic.fetch.shipNameField != null)
			{
				EditorLogic.fetch.shipNameField.onSelect.RemoveListener(VesselNaming(toggle: false));
				EditorLogic.fetch.shipNameField.onEndEdit.RemoveListener(VesselNaming(toggle: true));
			}
			if (EditorLogic.fetch != null && EditorLogic.fetch.shipDescriptionField != null)
			{
				EditorLogic.fetch.shipDescriptionField.onSelect.RemoveListener(VesselNaming(toggle: false));
				EditorLogic.fetch.shipDescriptionField.onEndEdit.RemoveListener(VesselNaming(toggle: true));
			}
			InputLockManager.RemoveControlLock("ActionGroupPanelOpen");
			if (Instance != null && Instance == this)
			{
				Instance = null;
			}
		}

		private void SelectGroup()
		{
			EditorActionGroup_Base editorActionGroup_Base = (EditorActionGroup_Base)actionGroupList.SelectedItem;
			selectedGroupType = editorActionGroup_Base.type;
			if (editorActionGroup_Base is EditorActionController)
			{
				selectedControllerId = ((EditorActionController)editorActionGroup_Base).PartId;
				selectedController = ((EditorActionController)editorActionGroup_Base).controller;
			}
			else
			{
				selectedGroup = ((EditorActionGroup)editorActionGroup_Base).Group;
			}
			ConstructLists(full: false);
		}

		public bool HasSelectedParts()
		{
			return selectedParts.Count > 0;
		}

		public void ClearSelection(bool reconstruct)
		{
			if (selectedParts == null)
			{
				selectedParts = new List<EditorActionPartSelector>();
			}
			int count = selectedParts.Count;
			while (count-- > 0)
			{
				if (selectedParts[count] != null)
				{
					selectedParts[count].Deselect();
				}
			}
			selectedParts.Clear();
			if (reconstruct)
			{
				ConstructLists(full: true);
			}
		}

		public bool SelectionContains(Part p)
		{
			int count = selectedParts.Count;
			while (true)
			{
				if (count-- > 0)
				{
					EditorActionPartSelector editorActionPartSelector = selectedParts[count];
					if (editorActionPartSelector.part == p)
					{
						break;
					}
					if (editorActionPartSelector.part.partInfo != null && !editorActionPartSelector.part.partInfo.mapActionsToSymmetryParts)
					{
						continue;
					}
					int count2 = editorActionPartSelector.part.symmetryCounterparts.Count;
					while (count2-- > 0)
					{
						if (editorActionPartSelector.part.symmetryCounterparts[count2] == p)
						{
							return true;
						}
					}
					continue;
				}
				return false;
			}
			return true;
		}

		public void AddToSelection(EditorActionPartSelector s)
		{
			if (!SelectionContains(s.part))
			{
				selectedParts.Add(s);
				ConstructLists(full: true);
			}
		}

		public List<Part> GetSelectedParts()
		{
			List<Part> list = new List<Part>();
			int count = selectedParts.Count;
			for (int i = 0; i < count; i++)
			{
				EditorActionPartSelector editorActionPartSelector = selectedParts[i];
				if (editorActionPartSelector != null && editorActionPartSelector.part != null)
				{
					list.Add(editorActionPartSelector.part);
				}
			}
			return list;
		}

		public void ResetPart(EditorActionPartSelector selector)
		{
			if (selector == null)
			{
				List<Part> list = GetSelectedParts();
				int count = list.Count;
				while (count-- > 0)
				{
					ResetPart(list[count], resetSym: true);
				}
			}
			else
			{
				ResetPart(selector.part, resetSym: true);
			}
			if (HighLogic.LoadedSceneIsEditor)
			{
				EditorLogic.fetch.SetBackup();
			}
			ConstructLists(full: true);
		}

		private void ResetPart(Part part, bool resetSym)
		{
			if (selectedGroupType == EditorActionGroupType.Action)
			{
				int count = part.Actions.Count;
				while (count-- > 0)
				{
					part.Actions[count].actionGroup = part.Actions[count].defaultActionGroup;
				}
				int count2 = part.Modules.Count;
				while (count2-- > 0)
				{
					PartModule partModule = part.Modules[count2];
					if (!partModule.isEnabled)
					{
						continue;
					}
					int count3 = partModule.Actions.Count;
					while (count3-- > 0)
					{
						BaseAction baseAction = partModule.Actions[count3];
						baseAction.actionGroup = baseAction.defaultActionGroup;
						for (int i = 0; i < baseAction.overrideGroups.Length; i++)
						{
							baseAction.overrideGroups[i] = KSPActionGroup.None;
						}
					}
				}
			}
			else
			{
				int count4 = part.Modules.Count;
				while (count4-- > 0)
				{
					PartModule partModule2 = part.Modules[count4];
					if (!partModule2.isEnabled)
					{
						continue;
					}
					int count5 = partModule2.Fields.Count;
					while (count5-- > 0)
					{
						if (partModule2.Fields[count5] is BaseAxisField baseAxisField)
						{
							baseAxisField.axisGroup = baseAxisField.defaultAxisGroup;
							for (int j = 0; j < baseAxisField.overrideGroups.Length; j++)
							{
								baseAxisField.overrideGroups[j] = KSPAxisGroup.None;
							}
						}
					}
				}
			}
			if (resetSym && (part.partInfo == null || part.partInfo.mapActionsToSymmetryParts))
			{
				int count6 = part.symmetryCounterparts.Count;
				while (count6-- > 0)
				{
					ResetPart(part.symmetryCounterparts[count6], resetSym: false);
				}
			}
		}

		private void ConstructActionList()
		{
			int setIndex = 0;
			lockGroupOverride = true;
			groupActionsList.Clear();
			groupActionsSelectables.Clear();
			if (selectedGroupType == EditorActionGroupType.Controller && selectedController != null)
			{
				GroupActionsHeading.text = Localizer.Format("#autoLOC_8003287");
				groupActionsList.CreateItem(actionControllerHeaderPrefab).gameObject.GetComponent<EditorActionControllerHeader>().Setup(selectedController.displayName, selectedController);
				groupActionsList.CreateItem(actionControllerOpenButtonPrefab).gameObject.GetComponent<EditorActionControllerOpenButton>().Setup(selectedController);
				ConstructGroupAxisList(0);
				ConstructGroupActionList(0);
				lockGroupOverride = false;
				UpdateMenuNavigationSelectables();
				return;
			}
			GroupActionsHeading.text = Localizer.Format("#autoLOC_900532");
			if (overrideGroupIndices == null || overrideGroupIndices.Length != overrideDefault.Length + 1)
			{
				overrideGroupIndices = new int[overrideDefault.Length + 1];
				overrideGroupOpen = new bool[overrideDefault.Length + 1];
				overrideGroupOpen[0] = true;
			}
			int num = overrideDefault.Length + 1;
			if (!GameSettings.ADDITIONAL_ACTION_GROUPS)
			{
				num = 1;
				selectedGroupOverride = 0;
				overrideGroupOpen[0] = true;
			}
			for (int i = 0; i < num; i++)
			{
				if (GameSettings.ADDITIONAL_ACTION_GROUPS)
				{
					overrideGroupIndices[i] = groupActionsList.Count;
					UISelectableGridLayoutGroupItem uISelectableGridLayoutGroupItem = groupActionsList.CreateItem(actionOverrideGroupHeaderPrefab);
					EditorActionOverrideGroup component = uISelectableGridLayoutGroupItem.gameObject.GetComponent<EditorActionOverrideGroup>();
					if (HighLogic.LoadedSceneIsFlight)
					{
						component.Setup(i, (i > 0) ? FlightGlobals.fetch.activeVessel.OverrideGroupNames : null, overrideGroupOpen[i]);
					}
					else
					{
						component.Setup(i, (i > 0) ? EditorLogic.fetch.ship.OverrideGroupNames : null, overrideGroupOpen[i]);
					}
					setIndex = uISelectableGridLayoutGroupItem.Index;
					groupActionsSelectables.Add(component.GetComponent<Selectable>());
				}
				if (overrideGroupOpen[i])
				{
					if (i > 0)
					{
						EditorActionOverrideToggle component2 = groupActionsList.CreateItem(actionOverrideTogglePrefab).gameObject.GetComponent<EditorActionOverrideToggle>();
						component2.Setup(i, overrideDefault[i - 1], isControl: false, "#autoLOC_6013018", isGroupAction: true, setIndex);
						component2.SetOverrideState = SetGroupOverrideState;
						bool flag;
						if ((!(flag = selectedGroupType == EditorActionGroupType.Axis) && ((uint)selectedGroup & 0x19u) != 0) || (flag && selectedGroup < 512))
						{
							component2 = groupActionsList.CreateItem(actionOverrideTogglePrefab).gameObject.GetComponent<EditorActionOverrideToggle>();
							int num2 = (flag ? ((int)overrideAxisControl[i - 1]) : ((int)overrideActionControl[i - 1]));
							component2.Setup(i, (num2 & selectedGroup) != 0, isControl: true, "#autoLOC_6013019", isGroupAction: true, setIndex);
							if (selectedGroupType == EditorActionGroupType.Action)
							{
								component2.SetOverrideState = SetControlActionOverrideState;
							}
							else
							{
								component2.SetOverrideState = SetControlAxisOverrideState;
							}
						}
					}
					if (selectedGroupType == EditorActionGroupType.Action)
					{
						ConstructGroupActionList(i);
					}
					else
					{
						ConstructGroupAxisList(i);
					}
				}
				if (GameSettings.ADDITIONAL_ACTION_GROUPS)
				{
					groupActionsList.CreateItem(separatorGroupHeaderPrefab);
				}
			}
			lockGroupOverride = false;
			if (GameSettings.ADDITIONAL_ACTION_GROUPS)
			{
				groupActionsList.Select(overrideGroupIndices[selectedGroupOverride]);
				groupActionsList.SelectSet(currentSelectedIndex);
			}
			else
			{
				groupActionsList.Select(overrideGroupIndices[0]);
				groupActionsList.SelectSet(0);
			}
			UpdateMenuNavigationSelectables();
		}

		private void ConstructPartList()
		{
			if (selectedGroupType == EditorActionGroupType.Controller)
			{
				ConstructPartAxisList(combinedList: true);
				ConstructPartActionList(combinedList: true);
			}
			else if (selectedGroupType == EditorActionGroupType.Action)
			{
				ConstructPartActionList();
			}
			else
			{
				ConstructPartAxisList();
			}
		}

		private void ConstructLists(bool full)
		{
			if (full)
			{
				ConstructGroupList();
			}
			ConstructActionList();
			ConstructPartList();
		}

		private List<bool> CreateGroups_Action()
		{
			SpaceCenterFacility spaceCenterFacility = SpaceCenterFacility.VehicleAssemblyBuilding;
			if (HighLogic.LoadedScene == GameScenes.EDITOR)
			{
				spaceCenterFacility = EditorDriver.editorFacility.ToFacility();
			}
			else
			{
				if (HighLogic.LoadedScene != GameScenes.FLIGHT)
				{
					return new List<bool>();
				}
				if (ScenarioUpgradeableFacilities.IsLaunchPad(FlightGlobals.ActiveVessel.launchedFrom))
				{
					spaceCenterFacility = SpaceCenterFacility.VehicleAssemblyBuilding;
				}
				else if (ScenarioUpgradeableFacilities.IsRunway(FlightGlobals.ActiveVessel.launchedFrom))
				{
					spaceCenterFacility = SpaceCenterFacility.SpaceplaneHangar;
				}
			}
			int actionGroupsLength = BaseAction.GetActionGroupsLength(ScenarioUpgradeableFacilities.GetFacilityLevel(spaceCenterFacility), spaceCenterFacility == SpaceCenterFacility.VehicleAssemblyBuilding);
			List<bool> list;
			if (HasSelectedParts())
			{
				list = BaseAction.CreateGroupList(GetSelectedParts(), selectedGroupOverride);
			}
			else
			{
				list = new List<bool>(actionGroupsLength);
				for (int i = 0; i < actionGroupsLength; i++)
				{
					list.Add(item: false);
				}
			}
			return list;
		}

		private List<bool> CreateGroups_Axis()
		{
			SpaceCenterFacility spaceCenterFacility = EditorDriver.editorFacility.ToFacility();
			int axisGroupsLength = BaseAxisField.GetAxisGroupsLength(ScenarioUpgradeableFacilities.GetFacilityLevel(spaceCenterFacility), spaceCenterFacility == SpaceCenterFacility.VehicleAssemblyBuilding);
			List<bool> list;
			if (HasSelectedParts())
			{
				list = BaseAxisField.CreateGroupList(GetSelectedParts(), selectedGroupOverride);
			}
			else
			{
				list = new List<bool>(axisGroupsLength);
				for (int i = 0; i < axisGroupsLength; i++)
				{
					list.Add(item: false);
				}
			}
			return list;
		}

		private List<bool> CreateGroups_Controller(List<ModuleRoboticController> controllers)
		{
			List<bool> list = new List<bool>(controllers.Count);
			List<Part> testParts = GetSelectedParts();
			for (int i = 0; i < controllers.Count; i++)
			{
				list.Add(controllers[i].HasPart(testParts));
			}
			return list;
		}

		private void ConstructGroupList()
		{
			actionGroupList.Clear();
			selectionSelectables.Clear();
			actiongroupSelectables.Clear();
			List<bool> list = CreateGroups_Action();
			List<bool> list2 = CreateGroups_Axis();
			int count = list.Count;
			int count2 = list2.Count;
			int itemIndex = 0;
			for (int i = 0; i < count; i++)
			{
				int num = 1 << i;
				((EditorActionGroup)actionGroupList.CreateItem(actionGroupPrefab)).Setup((KSPActionGroup)num, list[i]);
				if (selectedGroupType == EditorActionGroupType.Action && selectedGroup == num)
				{
					itemIndex = i;
				}
			}
			SetupAxisGroupsHeader();
			for (int j = 0; j < count2; j++)
			{
				int num2 = 1 << j;
				((EditorActionGroup)actionGroupList.CreateItem(actionGroupPrefab)).Setup((KSPAxisGroup)num2, list2[j]);
				if (selectedGroupType == EditorActionGroupType.Axis && selectedGroup == num2)
				{
					itemIndex = j + count;
				}
			}
			if (ExpansionsLoader.IsExpansionInstalled("Serenity"))
			{
				SetupControllersHeader();
				List<ModuleRoboticController> shipControllers = GetShipControllers();
				List<bool> list3 = CreateGroups_Controller(shipControllers);
				for (int k = 0; k < shipControllers.Count; k++)
				{
					((EditorActionController)actionGroupList.CreateItem(actionControllerPrefab)).Setup(shipControllers[k], list3[k]);
					if (selectedGroupType == EditorActionGroupType.Controller && selectedControllerId == shipControllers[k].PartPersistentId)
					{
						itemIndex = k + count + count2;
					}
				}
			}
			actionGroupList.Select(itemIndex);
			UpdateMenuNavigationSelectables();
		}

		private List<ModuleRoboticController> GetShipControllers()
		{
			if (cacheControllers == null)
			{
				cacheControllers = new List<ModuleRoboticController>();
			}
			else
			{
				cacheControllers.Clear();
			}
			ModuleRoboticController controller;
			if (HighLogic.LoadedSceneIsFlight)
			{
				if (FlightGlobals.fetch != null && FlightGlobals.fetch.activeVessel != null)
				{
					for (int i = 0; i < FlightGlobals.fetch.activeVessel.parts.Count; i++)
					{
						if (FlightGlobals.fetch.activeVessel.parts[i].isRoboticController(out controller))
						{
							cacheControllers.Add(controller);
						}
					}
				}
			}
			else if (EditorLogic.fetch != null && EditorLogic.fetch.ship != null)
			{
				for (int j = 0; j < EditorLogic.fetch.ship.parts.Count; j++)
				{
					if (EditorLogic.fetch.ship.parts[j].isRoboticController(out controller))
					{
						cacheControllers.Add(controller);
					}
				}
			}
			return cacheControllers;
		}

		private void SetupAxisGroupsHeader()
		{
			if (axisGroupHeader == null)
			{
				axisGroupHeader = Object.Instantiate(actionGroupColumnHeaderPrefab);
				if (axisGroupHeader != null)
				{
					axisGroupHeader.gameObject.transform.SetParent(actionGroupList.transform);
					axisGroupHeader.text = Localizer.Format("#autoLOC_8003261");
				}
			}
			else
			{
				axisGroupHeader.gameObject.transform.SetAsLastSibling();
			}
		}

		private void SetupControllersHeader()
		{
			if (controllersHeader == null)
			{
				controllersHeader = Object.Instantiate(actionGroupColumnHeaderPrefab);
				if (controllersHeader != null)
				{
					controllersHeader.gameObject.transform.SetParent(actionGroupList.transform);
					controllersHeader.text = Localizer.Format("#autoLOC_8003262");
				}
			}
			else
			{
				controllersHeader.gameObject.transform.SetAsLastSibling();
			}
		}

		private void ConstructGroupActionList(int overrideGroup)
		{
			KSPActionGroup group = (KSPActionGroup)selectedGroup;
			uint num = selectedControllerId;
			ModuleRoboticController controller = selectedController;
			List<BaseAction> list = new List<BaseAction>();
			list = ((selectedGroupType == EditorActionGroupType.Controller) ? ((!HighLogic.LoadedSceneIsFlight) ? BaseAction.CreateActionList(EditorLogic.fetch.ship.parts, controller, include: true) : BaseAction.CreateActionList(FlightGlobals.fetch.activeVessel.parts, controller, include: true)) : ((!HighLogic.LoadedSceneIsFlight) ? BaseAction.CreateActionList(EditorLogic.fetch.ship.parts, group, overrideGroup, overrideDefault: true, include: true) : BaseAction.CreateActionList(FlightGlobals.fetch.activeVessel.parts, group, overrideGroup, overrideDefault: true, include: true)));
			HashSet<Part> hashSet = new HashSet<Part>();
			Part part = null;
			int num2 = 0;
			int num3 = 0;
			EditorActionPartItem editorActionPartItem = null;
			EditorActionPartSelector editorActionPartSelector = null;
			bool flag = false;
			int count = list.Count;
			for (int i = 0; i < count; i++)
			{
				BaseAction baseAction = list[i];
				if (baseAction.listParent.part != part)
				{
					if (hashSet.Contains(baseAction.listParent.part))
					{
						continue;
					}
					part = baseAction.listParent.part;
					if (baseAction.listParent.part.partInfo == null || baseAction.listParent.part.partInfo.mapActionsToSymmetryParts)
					{
						int count2 = part.symmetryCounterparts.Count;
						for (int j = 0; j < count2; j++)
						{
							Part item = part.symmetryCounterparts[j];
							if (!hashSet.Contains(item))
							{
								hashSet.Add(item);
							}
						}
					}
					editorActionPartSelector = part.GetComponent<EditorActionPartSelector>();
					editorActionPartItem = (EditorActionPartItem)groupActionsList.CreateItem(groupPartTitlePrefab);
					editorActionPartItem.Setup(editorActionPartSelector.part.partInfo.title, group, overrideGroup, editorActionPartSelector, null, addToGroup: false);
					if (selectedGroupType == EditorActionGroupType.Controller)
					{
						if (editorActionPartSelector.part.isRoboticController(out var controller2))
						{
							editorActionPartItem.Setup(controller2.displayName, num, editorActionPartSelector, null, null, addToGroup: false);
						}
						else
						{
							editorActionPartItem.Setup(editorActionPartSelector.part.partInfo.title, num, editorActionPartSelector, null, null, addToGroup: false);
						}
					}
					else
					{
						editorActionPartItem.Setup(editorActionPartSelector.part.partInfo.title, group, overrideGroup, editorActionPartSelector, null, addToGroup: false);
					}
					if (!flag && selectedParts.Contains(editorActionPartSelector))
					{
						num3 = num2;
						flag = true;
					}
					num2++;
				}
				GameObject gameObject = Object.Instantiate(groupPartActionPrefab);
				editorActionPartItem = gameObject.GetComponentInChildren<EditorActionPartItem>();
				groupActionsList.AddItem(editorActionPartItem, gameObject.transform);
				if (selectedGroupType == EditorActionGroupType.Controller)
				{
					editorActionPartItem.Setup(baseAction.guiName, num, editorActionPartSelector, null, baseAction, addToGroup: false);
				}
				else
				{
					editorActionPartItem.Setup(baseAction.guiName, group, overrideGroup, editorActionPartSelector, baseAction, addToGroup: false);
				}
				num2++;
			}
			int num4 = num3;
			if (selectedGroupType == EditorActionGroupType.Controller)
			{
				num4 += 2;
			}
			if (groupActionsList.Count > num4)
			{
				groupActionsList.Select(num4);
			}
			UpdateMenuNavigationSelectables();
		}

		private void ConstructGroupAxisList(int overrideGroup)
		{
			KSPAxisGroup group = (KSPAxisGroup)selectedGroup;
			uint num = selectedControllerId;
			ModuleRoboticController controller = selectedController;
			BaseAxisFieldList baseAxisFieldList = ((selectedGroupType == EditorActionGroupType.Controller) ? ((!HighLogic.LoadedSceneIsFlight) ? BaseAxisField.CreateAxisList(EditorLogic.fetch.ship.parts, controller, include: true) : BaseAxisField.CreateAxisList(FlightGlobals.fetch.activeVessel.parts, controller, include: true)) : ((!HighLogic.LoadedSceneIsFlight) ? BaseAxisField.CreateAxisList(EditorLogic.fetch.ship.parts, group, overrideGroup, overrideDefault: true, include: true) : BaseAxisField.CreateAxisList(FlightGlobals.fetch.activeVessel.parts, group, overrideGroup, overrideDefault: true, include: true)));
			HashSet<Part> hashSet = new HashSet<Part>();
			Part part = null;
			int num2 = 0;
			int num3 = 0;
			EditorActionPartItem editorActionPartItem = null;
			EditorActionPartSelector editorActionPartSelector = null;
			bool flag = false;
			int count = baseAxisFieldList.Count;
			for (int i = 0; i < count; i++)
			{
				BaseAxisField baseAxisField = baseAxisFieldList[i];
				if (baseAxisField.module.part != part)
				{
					part = baseAxisField.module.part;
					if (part.partInfo == null || part.partInfo.mapActionsToSymmetryParts)
					{
						int count2 = part.symmetryCounterparts.Count;
						for (int j = 0; j < count2; j++)
						{
							if (!hashSet.Contains(baseAxisField.module.part))
							{
								Part item = part.symmetryCounterparts[j];
								if (!hashSet.Contains(item))
								{
									hashSet.Add(item);
								}
							}
						}
					}
					editorActionPartSelector = part.GetComponent<EditorActionPartSelector>();
					if (!hashSet.Contains(baseAxisField.module.part))
					{
						editorActionPartItem = (EditorActionPartItem)groupActionsList.CreateItem(groupPartTitlePrefab);
						if (selectedGroupType == EditorActionGroupType.Controller)
						{
							if (editorActionPartSelector.part.isRoboticController(out var controller2))
							{
								editorActionPartItem.Setup(controller2.displayName, num, editorActionPartSelector, null, null, addToGroup: false);
							}
							else
							{
								editorActionPartItem.Setup(editorActionPartSelector.part.partInfo.title, num, editorActionPartSelector, null, null, addToGroup: false);
							}
						}
						else
						{
							editorActionPartItem.Setup(editorActionPartSelector.part.partInfo.title, group, overrideGroup, editorActionPartSelector, null, addToGroup: false);
						}
					}
					if (!flag && selectedParts.Contains(editorActionPartSelector))
					{
						num3 = num2;
						flag = true;
					}
					num2++;
				}
				if (!hashSet.Contains(baseAxisField.module.part))
				{
					GameObject gameObject = Object.Instantiate(groupPartActionPrefab);
					editorActionPartItem = gameObject.GetComponentInChildren<EditorActionPartItem>();
					groupActionsList.AddItem(editorActionPartItem, gameObject.transform);
					if (selectedGroupType == EditorActionGroupType.Controller)
					{
						editorActionPartItem.Setup(baseAxisField.guiName, num, editorActionPartSelector, baseAxisField, null, addToGroup: false);
					}
					else
					{
						editorActionPartItem.Setup(baseAxisField.guiName, group, overrideGroup, editorActionPartSelector, baseAxisField, addToGroup: false);
					}
				}
				num2++;
			}
			int num4 = num3;
			if (selectedGroupType == EditorActionGroupType.Controller)
			{
				num3 += 2;
			}
			if (groupActionsList.Count > num4)
			{
				groupActionsList.Select(num4);
			}
			UpdateMenuNavigationSelectables();
		}

		private void ConstructPartActionList()
		{
			ConstructPartActionList(combinedList: false);
		}

		private void ConstructPartActionList(bool combinedList)
		{
			if (!combinedList)
			{
				partActionList.Clear();
			}
			if (!HasSelectedParts())
			{
				return;
			}
			selectionSelectables.Clear();
			KSPActionGroup group = (KSPActionGroup)selectedGroup;
			uint num = selectedControllerId;
			ModuleRoboticController controller = selectedController;
			List<BaseAction> list = ((selectedGroupType != EditorActionGroupType.Controller) ? BaseAction.CreateActionList(GetSelectedParts(), group, selectedGroupOverride, OverrideDefault, include: false) : BaseAction.CreateActionList(GetSelectedParts(), controller, include: false));
			HashSet<Part> hashSet = new HashSet<Part>();
			Part part = null;
			EditorActionPartSelector editorActionPartSelector = null;
			EditorActionPartItem editorActionPartItem = null;
			int num2 = 0;
			int count = list.Count;
			int count2;
			for (int i = 0; i < count; i++)
			{
				BaseAction baseAction = list[i];
				if (baseAction.noLongerAssignable)
				{
					continue;
				}
				if (baseAction.listParent.part != part)
				{
					if (hashSet.Contains(baseAction.listParent.part))
					{
						continue;
					}
					if (part != null && BaseAction.ContainsNonDefaultActions(part))
					{
						partActionList.CreateItem(partActionResetPrefab).gameObject.GetComponent<EditorActionPartReset>().Setup(editorActionPartSelector);
						num2++;
					}
					part = baseAction.listParent.part;
					if (!hashSet.Contains(part))
					{
						hashSet.Add(part);
					}
					if (part.partInfo == null || part.partInfo.mapActionsToSymmetryParts)
					{
						count2 = part.symmetryCounterparts.Count;
						for (int j = 0; j < count2; j++)
						{
							Part item = part.symmetryCounterparts[j];
							if (!hashSet.Contains(item))
							{
								hashSet.Add(item);
							}
						}
					}
					editorActionPartSelector = part.GetComponent<EditorActionPartSelector>();
					editorActionPartItem = partActionList.CreateItem(partActionTitlePrefab).gameObject.GetComponent<EditorActionPartItem>();
					if (selectedGroupType == EditorActionGroupType.Controller)
					{
						if (editorActionPartSelector.part.isRoboticController(out var controller2))
						{
							editorActionPartItem.Setup(controller2.displayName, num, editorActionPartSelector, null, null, addToGroup: false);
						}
						else
						{
							editorActionPartItem.Setup(editorActionPartSelector.part.partInfo.title, num, editorActionPartSelector, null, null, addToGroup: false);
						}
						editorActionPartItem = partActionList.CreateItem(partActionTextPrefab).gameObject.GetComponent<EditorActionPartItem>();
						editorActionPartItem.Setup(Localizer.Format("#autoLOC_8003346"), num, editorActionPartSelector, null, null, addToGroup: false);
					}
					else
					{
						editorActionPartItem.Setup(editorActionPartSelector.part.partInfo.title, group, 0, editorActionPartSelector, null, addToGroup: false);
					}
				}
				editorActionPartItem = partActionList.CreateItem(partActionPrefab).gameObject.GetComponent<EditorActionPartItem>();
				if (selectedGroupType == EditorActionGroupType.Controller)
				{
					editorActionPartItem.Setup(baseAction.guiName, num, editorActionPartSelector, null, baseAction, addToGroup: true);
				}
				else
				{
					editorActionPartItem.Setup(baseAction.guiName, group, 0, editorActionPartSelector, baseAction, addToGroup: true);
				}
			}
			if (part != null && BaseAction.ContainsNonDefaultActions(part))
			{
				partActionList.CreateItem(partActionResetPrefab).gameObject.GetComponent<EditorActionPartReset>().Setup(editorActionPartSelector);
				num2++;
			}
			List<Part> list2 = new List<Part>(GetSelectedParts());
			int count3 = list2.Count;
			while (count3-- > 0)
			{
				if (hashSet.Contains(list2[count3]))
				{
					list2.RemoveAt(count3);
				}
			}
			count2 = list2.Count;
			for (int k = 0; k < count2; k++)
			{
				Part item = list2[k];
				editorActionPartSelector = item.GetComponent<EditorActionPartSelector>();
				editorActionPartItem = partActionList.CreateItem(partActionTitlePrefab).gameObject.GetComponent<EditorActionPartItem>();
				if (selectedGroupType == EditorActionGroupType.Controller)
				{
					if (editorActionPartSelector.part.isRoboticController(out var controller3))
					{
						editorActionPartItem.Setup(controller3.displayName, num, editorActionPartSelector, null, null, addToGroup: false);
					}
					else
					{
						editorActionPartItem.Setup(editorActionPartSelector.part.partInfo.title, num, editorActionPartSelector, null, null, addToGroup: false);
					}
					editorActionPartItem = partActionList.CreateItem(partActionTextPrefab).gameObject.GetComponent<EditorActionPartItem>();
					editorActionPartItem.Setup(Localizer.Format("#autoLOC_8003346"), num, editorActionPartSelector, null, null, addToGroup: false);
				}
				else
				{
					editorActionPartItem.Setup(editorActionPartSelector.part.partInfo.title, group, 0, editorActionPartSelector, null, addToGroup: false);
				}
				if (BaseAction.ContainsNonDefaultActions(item))
				{
					partActionList.CreateItem(partActionResetPrefab).gameObject.GetComponent<EditorActionPartReset>().Setup(editorActionPartSelector);
					num2++;
				}
			}
			if (num2 > 1)
			{
				partActionList.CreateItem(partActionResetPrefab).gameObject.GetComponent<EditorActionPartReset>().Setup(null);
			}
			partActionList.Select(0);
			UpdateMenuNavigationSelectables();
		}

		private void ConstructPartAxisList()
		{
			ConstructPartAxisList(combinedList: false);
		}

		private void ConstructPartAxisList(bool combinedList)
		{
			partActionList.Clear();
			if (!HasSelectedParts())
			{
				return;
			}
			KSPAxisGroup group = (KSPAxisGroup)selectedGroup;
			uint num = selectedControllerId;
			ModuleRoboticController controller = selectedController;
			BaseAxisFieldList baseAxisFieldList = ((selectedGroupType != EditorActionGroupType.Controller) ? BaseAxisField.CreateAxisList(GetSelectedParts(), group, selectedGroupOverride, OverrideDefault, include: false) : BaseAxisField.CreateAxisList(GetSelectedParts(), controller, include: false));
			List<Part> list = new List<Part>();
			Part part = null;
			EditorActionPartSelector editorActionPartSelector = null;
			EditorActionPartItem editorActionPartItem = null;
			int num2 = 0;
			int count = baseAxisFieldList.Count;
			int count2;
			for (int i = 0; i < count; i++)
			{
				BaseAxisField baseAxisField = baseAxisFieldList[i];
				if (baseAxisField.module.part != part)
				{
					if (list.Contains(baseAxisField.module.part))
					{
						continue;
					}
					if (part != null && BaseAxisField.ContainsNonDefaultAxes(part))
					{
						partActionList.CreateItem(partActionResetPrefab).gameObject.GetComponent<EditorActionPartReset>().Setup(editorActionPartSelector);
						num2++;
					}
					part = baseAxisField.module.part;
					if (!list.Contains(part))
					{
						list.Add(part);
					}
					if (part.partInfo == null || part.partInfo.mapActionsToSymmetryParts)
					{
						count2 = part.symmetryCounterparts.Count;
						for (int j = 0; j < count2; j++)
						{
							Part item = part.symmetryCounterparts[j];
							if (!list.Contains(item))
							{
								list.Add(item);
							}
						}
					}
					editorActionPartSelector = part.GetComponent<EditorActionPartSelector>();
					editorActionPartItem = partActionList.CreateItem(partActionTitlePrefab).gameObject.GetComponent<EditorActionPartItem>();
					if (selectedGroupType == EditorActionGroupType.Controller)
					{
						if (editorActionPartSelector.part.isRoboticController(out var controller2))
						{
							editorActionPartItem.Setup(controller2.displayName, num, editorActionPartSelector, null, null, addToGroup: false);
						}
						else
						{
							editorActionPartItem.Setup(editorActionPartSelector.part.partInfo.title, num, editorActionPartSelector, null, null, addToGroup: false);
						}
						editorActionPartItem = partActionList.CreateItem(partActionTextPrefab).gameObject.GetComponent<EditorActionPartItem>();
						editorActionPartItem.Setup(Localizer.Format("#autoLOC_8003347"), num, editorActionPartSelector, null, null, addToGroup: false);
					}
					else
					{
						editorActionPartItem.Setup(editorActionPartSelector.part.partInfo.title, group, 0, editorActionPartSelector, null, addToGroup: false);
					}
				}
				editorActionPartItem = partActionList.CreateItem(partActionPrefab).gameObject.GetComponent<EditorActionPartItem>();
				if (selectedGroupType == EditorActionGroupType.Controller)
				{
					editorActionPartItem.Setup(baseAxisField.guiName, num, editorActionPartSelector, baseAxisField, null, addToGroup: true);
				}
				else
				{
					editorActionPartItem.Setup(baseAxisField.guiName, group, 0, editorActionPartSelector, baseAxisField, addToGroup: true);
				}
			}
			if (part != null && BaseAxisField.ContainsNonDefaultAxes(part))
			{
				partActionList.CreateItem(partActionResetPrefab).gameObject.GetComponent<EditorActionPartReset>().Setup(editorActionPartSelector);
				num2++;
			}
			List<Part> list2 = new List<Part>(GetSelectedParts());
			int count3 = list2.Count;
			while (count3-- > 0)
			{
				if (list.Contains(list2[count3]))
				{
					list2.RemoveAt(count3);
				}
			}
			count2 = list2.Count;
			for (int k = 0; k < count2; k++)
			{
				Part item = list2[k];
				editorActionPartSelector = item.GetComponent<EditorActionPartSelector>();
				editorActionPartItem = partActionList.CreateItem(partActionTitlePrefab).gameObject.GetComponent<EditorActionPartItem>();
				if (selectedGroupType == EditorActionGroupType.Controller)
				{
					if (editorActionPartSelector.part.isRoboticController(out var controller3))
					{
						editorActionPartItem.Setup(controller3.displayName, num, editorActionPartSelector, null, null, addToGroup: false);
					}
					else
					{
						editorActionPartItem.Setup(editorActionPartSelector.part.partInfo.title, num, editorActionPartSelector, null, null, addToGroup: false);
					}
					editorActionPartItem = partActionList.CreateItem(partActionTextPrefab).gameObject.GetComponent<EditorActionPartItem>();
					editorActionPartItem.Setup(Localizer.Format("#autoLOC_8003347"), num, editorActionPartSelector, null, null, addToGroup: false);
				}
				else
				{
					editorActionPartItem.Setup(editorActionPartSelector.part.partInfo.title, group, 0, editorActionPartSelector, null, addToGroup: false);
				}
				if (BaseAxisField.ContainsNonDefaultAxes(item))
				{
					partActionList.CreateItem(partActionResetPrefab).gameObject.GetComponent<EditorActionPartReset>().Setup(editorActionPartSelector);
					num2++;
				}
			}
			if (num2 > 1)
			{
				partActionList.CreateItem(partActionResetPrefab).gameObject.GetComponent<EditorActionPartReset>().Setup(null);
			}
			partActionList.Select(0);
			UpdateMenuNavigationSelectables();
		}

		private void AddToGroup(BaseAction action, KSPActionGroup group)
		{
			if (selectedGroupOverride > 0)
			{
				action.overrideGroups[selectedGroupOverride - 1] |= group;
			}
			else
			{
				action.actionGroup |= group;
			}
		}

		private void AddToGroup(EditorActionPartItem item, KSPActionGroup group)
		{
			AddToGroup(item.evt, group);
			if (item.evt.listParent.part.partInfo != null && !item.evt.listParent.part.partInfo.mapActionsToSymmetryParts)
			{
				return;
			}
			if (item.evt.listParent.module != null)
			{
				int index = item.evt.listParent.part.Modules.IndexOf(item.evt.listParent.module);
				int count = item.evt.listParent.part.symmetryCounterparts.Count;
				for (int i = 0; i < count; i++)
				{
					PartModule partModule = item.evt.listParent.part.symmetryCounterparts[i].Modules[index];
					if (!(partModule == null))
					{
						BaseAction baseAction = partModule.Actions[item.evt.name];
						if (baseAction != null)
						{
							AddToGroup(baseAction, group);
						}
					}
				}
				return;
			}
			int count2 = item.evt.listParent.part.symmetryCounterparts.Count;
			for (int j = 0; j < count2; j++)
			{
				BaseAction baseAction2 = item.evt.listParent.part.symmetryCounterparts[j].Actions[item.evt.name];
				if (baseAction2 != null)
				{
					AddToGroup(baseAction2, group);
				}
			}
		}

		private void AddToGroup(BaseAxisField axisField, KSPAxisGroup group)
		{
			if (selectedGroupOverride > 0)
			{
				axisField.overrideGroups[selectedGroupOverride - 1] |= group;
			}
			else
			{
				axisField.axisGroup |= group;
			}
		}

		private void AddToGroup(EditorActionPartItem item, KSPAxisGroup group)
		{
			AddToGroup(item.axisField, group);
			if (item.axisField.module.part.partInfo != null && !item.axisField.module.part.partInfo.mapActionsToSymmetryParts)
			{
				return;
			}
			int index = item.axisField.module.part.Modules.IndexOf(item.axisField.module);
			int count = item.axisField.module.part.symmetryCounterparts.Count;
			for (int i = 0; i < count; i++)
			{
				PartModule partModule = item.axisField.module.part.symmetryCounterparts[i].Modules[index];
				if (!(partModule == null) && partModule.Fields[item.axisField.name] is BaseAxisField axisField)
				{
					AddToGroup(axisField, group);
				}
			}
		}

		private void AddToController(EditorActionPartItem item, ModuleRoboticController controller)
		{
			if (item.evt == null)
			{
				controller.AddPartAxis(item.axisField.module.part, item.axisField.module, item.axisField);
			}
			else
			{
				controller.AddPartAction(item.selector.part, item.evt.listParent.module, item.evt);
			}
		}

		public void AddActionToGroup(EditorActionPartItem item)
		{
			if (item.selectedGroupType == EditorActionGroupType.Controller)
			{
				AddToController(item, selectedController);
			}
			else if (item.selectedGroupType == EditorActionGroupType.Axis)
			{
				AddToGroup(item, (KSPAxisGroup)selectedGroup);
			}
			else
			{
				AddToGroup(item, (KSPActionGroup)selectedGroup);
			}
			if (HighLogic.LoadedSceneIsEditor)
			{
				EditorLogic.fetch.SetBackup();
			}
			ConstructLists(full: true);
		}

		private void RemoveFromGroup(BaseAction action, KSPActionGroup group)
		{
			if (selectedGroupOverride > 0)
			{
				action.overrideGroups[selectedGroupOverride - 1] &= ~group;
			}
			else
			{
				action.actionGroup &= ~group;
			}
		}

		private void RemoveFromGroup(EditorActionPartItem item, KSPActionGroup group)
		{
			RemoveFromGroup(item.evt, group);
			if (item.evt.listParent.part.partInfo != null && !item.evt.listParent.part.partInfo.mapActionsToSymmetryParts)
			{
				return;
			}
			if (item.evt.listParent.module != null)
			{
				int index = item.evt.listParent.part.Modules.IndexOf(item.evt.listParent.module);
				int count = item.evt.listParent.part.symmetryCounterparts.Count;
				for (int i = 0; i < count; i++)
				{
					PartModule partModule = item.evt.listParent.part.symmetryCounterparts[i].Modules[index];
					if (!(partModule == null))
					{
						BaseAction baseAction = partModule.Actions[item.evt.name];
						if (baseAction != null)
						{
							RemoveFromGroup(baseAction, group);
						}
					}
				}
				return;
			}
			int count2 = item.evt.listParent.part.symmetryCounterparts.Count;
			for (int j = 0; j < count2; j++)
			{
				BaseAction baseAction2 = item.evt.listParent.part.symmetryCounterparts[j].Actions[item.evt.name];
				if (baseAction2 != null)
				{
					RemoveFromGroup(baseAction2, group);
				}
			}
		}

		private void RemoveFromGroup(BaseAxisField axisField, KSPAxisGroup group)
		{
			if (selectedGroupOverride > 0)
			{
				axisField.overrideGroups[selectedGroupOverride - 1] &= ~group;
			}
			else
			{
				axisField.axisGroup &= ~group;
			}
		}

		private void RemoveFromGroup(EditorActionPartItem item, KSPAxisGroup group)
		{
			RemoveFromGroup(item.axisField, group);
			if (item.axisField.module.part.partInfo != null && !item.axisField.module.part.partInfo.mapActionsToSymmetryParts)
			{
				return;
			}
			int index = item.axisField.module.part.Modules.IndexOf(item.axisField.module);
			int count = item.axisField.module.part.symmetryCounterparts.Count;
			for (int i = 0; i < count; i++)
			{
				PartModule partModule = item.axisField.module.part.symmetryCounterparts[i].Modules[index];
				if (!(partModule == null) && partModule.Fields[item.axisField.name] is BaseAxisField axisField)
				{
					RemoveFromGroup(axisField, group);
				}
			}
		}

		private void RemoveFromController(EditorActionPartItem item, ModuleRoboticController controller, bool transferToSymPartner)
		{
			if (item.evt == null)
			{
				controller.RemovePartAxis(item.axisField.module.part, item.axisField, transferToSymPartner);
			}
			else
			{
				controller.RemovePartAction(item.selector.part, item.evt, transferToSymPartner);
			}
		}

		public void RemoveActionFromGroup(EditorActionPartItem item)
		{
			if (item.selectedGroupType == EditorActionGroupType.Controller)
			{
				RemoveFromController(item, selectedController, transferToSymPartner: false);
			}
			else if (item.selectedGroupType == EditorActionGroupType.Axis)
			{
				RemoveFromGroup(item, (KSPAxisGroup)selectedGroup);
			}
			else
			{
				RemoveFromGroup(item, (KSPActionGroup)selectedGroup);
			}
			if (HighLogic.LoadedSceneIsEditor)
			{
				EditorLogic.fetch.SetBackup();
			}
			ConstructLists(full: true);
		}

		public void CloseGroup(int groupOverride)
		{
			if (groupOverride < 0 || groupOverride > overrideDefault.Length)
			{
				groupOverride = 0;
			}
			overrideGroupOpen[groupOverride] = false;
			ConstructLists(full: false);
		}

		public void SetGroupOverride(int groupOverride)
		{
			if (!lockGroupOverride)
			{
				selectedGroupOverride = groupOverride;
				if (selectedGroupOverride < 0 || selectedGroupOverride > overrideDefault.Length)
				{
					selectedGroupOverride = 0;
				}
				overrideGroupOpen[selectedGroupOverride] = true;
				groupActionsList.Select(overrideGroupIndices[selectedGroupOverride]);
				ConstructLists(full: true);
			}
		}

		public void SetGroupOverrideState(int groupOverride, bool on)
		{
			if (groupOverride < 0 || groupOverride > overrideDefault.Length)
			{
				groupOverride = 0;
			}
			overrideDefault[groupOverride - 1] = on;
			SetGroupOverride(groupOverride);
		}

		public void SetControlActionOverrideState(int groupOverride, bool on)
		{
			if (groupOverride < 0 || groupOverride > overrideDefault.Length)
			{
				groupOverride = 0;
			}
			KSPActionGroup kSPActionGroup = (KSPActionGroup)selectedGroup;
			if (on)
			{
				overrideActionControl[groupOverride - 1] |= kSPActionGroup;
			}
			else
			{
				overrideActionControl[groupOverride - 1] &= ~kSPActionGroup;
			}
			SetGroupOverride(groupOverride);
		}

		public void SetControlAxisOverrideState(int groupOverride, bool on)
		{
			if (groupOverride < 0 || groupOverride > overrideDefault.Length)
			{
				groupOverride = 0;
			}
			KSPAxisGroup kSPAxisGroup = (KSPAxisGroup)selectedGroup;
			if (on)
			{
				overrideAxisControl[groupOverride - 1] |= kSPAxisGroup;
			}
			else
			{
				overrideAxisControl[groupOverride - 1] &= ~kSPAxisGroup;
			}
			SetGroupOverride(groupOverride);
		}

		public void ActivateInterface(ShipConstruct ship)
		{
			interfaceActive = true;
			EditorActionPartSelector.CreatePartActions(ship.parts);
			ClearSelection(reconstruct: true);
			ConstructGroupList();
			InputLockManager.SetControlLock(ControlTypes.GUI, "EditorActionGroups_CameraLock");
			if (menuNavigation != null)
			{
				StartCoroutine(ToggleMenuNavigationInputLock(state: false));
				SetMenuNavEnabled(enabled: true);
			}
			GameEvents.onGUIActionGroupShown.Fire();
		}

		public void DeactivateInterface(ShipConstruct ship)
		{
			ClearSelection(reconstruct: false);
			InputLockManager.RemoveControlLock("EditorActionGroups_CameraLock");
			EditorActionPartSelector.DestroyPartActions(ship.parts);
			if (menuNavigation != null)
			{
				menuNavigation.SetMenuNavInputLock(newState: false);
				SetMenuNavEnabled(enabled: false);
			}
			interfaceActive = false;
			GameEvents.onGUIActionGroupClosed.Fire();
		}

		public void ActivateInFlightInterface(Vessel ship)
		{
			EditorActionPartSelector.CreatePartActions(ship.parts);
			ClearSelection(reconstruct: true);
			ConstructGroupList();
			interfaceActive = true;
			if (menuNavigation != null)
			{
				SetMenuNavEnabled(enabled: true);
				StartCoroutine(ToggleMenuNavigationInputLock(state: false));
				if (menuNavigation.selectableItems.Count > 0 && menuNavigation.selectableItems[0] != null)
				{
					menuNavigation.SetItemAsFirstSelected(menuNavigation.selectableItems[0].gameObject);
				}
			}
			interfaceActive = true;
		}

		public void DectivateInFlightInterface(Vessel ship)
		{
			ClearSelection(reconstruct: false);
			interfaceActive = false;
			if (menuNavigation != null)
			{
				SetMenuNavEnabled(enabled: false);
			}
		}

		private void OnEditorUndoRedo(ShipConstruct ship)
		{
			ClearSelection(EditorLogic.fetch.editorScreen == EditorScreen.Actions);
		}

		private void ToggleAdditionalActions(bool on)
		{
			GameSettings.ADDITIONAL_ACTION_GROUPS = on;
			GameSettings.SaveSettingsOnNextGameSave();
			ConstructLists(full: false);
		}

		private void OnControllerAxesOrActionsChanged(ModuleRoboticController controller)
		{
			if (HighLogic.LoadedSceneIsEditor && EditorLogic.fetch.editorScreen == EditorScreen.Actions)
			{
				RebuildLists(fullRebuild: false, keepSelection: true);
			}
			else if (HighLogic.LoadedSceneIsFlight && ActionGroupsFlightController.Instance.IsOpen)
			{
				RebuildLists(fullRebuild: false, keepSelection: true);
			}
		}

		internal void SelectController(ModuleRoboticController controller)
		{
			List<bool> list = CreateGroups_Action();
			List<bool> list2 = CreateGroups_Axis();
			List<ModuleRoboticController> shipControllers = GetShipControllers();
			for (int i = 0; i < shipControllers.Count; i++)
			{
				if (controller.PartPersistentId == shipControllers[i].PartPersistentId)
				{
					actionGroupList.Select(i + list.Count + list2.Count);
					SelectGroup();
				}
			}
		}

		internal void RebuildLists(bool fullRebuild)
		{
			ClearSelection(fullRebuild);
		}

		internal void RebuildLists(bool fullRebuild, bool keepSelection)
		{
			if (keepSelection)
			{
				ConstructLists(fullRebuild);
			}
			else
			{
				ClearSelection(fullRebuild);
			}
		}

		public void OnPointerEnter(PointerEventData eventData)
		{
			isMouseOver = true;
		}

		public void OnPointerExit(PointerEventData eventData)
		{
			isMouseOver = false;
		}

		private void InitializeMenuNavigation()
		{
			Transform transform = TransformExtension.FindChild(base.transform, "BackgroundMain");
			if (transform != null)
			{
				Button component = transform.GetComponent<Button>();
				if (component != null)
				{
					component.interactable = false;
				}
			}
			if (HighLogic.LoadedSceneIsFlight)
			{
				Transform transform2 = TransformExtension.FindChild(base.transform, "anchorActionGroups");
				if (transform2 != null)
				{
					Button component2 = transform2.GetComponent<Button>();
					if (component2 != null)
					{
						component2.interactable = false;
					}
				}
				Transform transform3 = TransformExtension.FindChild(base.transform, "ButtonExit");
				if (transform3 != null)
				{
					flightExitButton = transform3.GetComponent<Button>();
				}
			}
			menuNavigation = MenuNavigation.SpawnMenuNavigation(base.gameObject, Navigation.Mode.Vertical, limitCheck: true);
			if (HighLogic.LoadedSceneIsEditor)
			{
				StartCoroutine(ToggleMenuNavigationInputLock(state: false));
			}
			SetMenuNavEnabled(enabled: false);
			if (HighLogic.LoadedSceneIsEditor)
			{
				if (EditorLogic.fetch.shipNameField != null)
				{
					EditorLogic.fetch.shipNameField.onSelect.AddListener(VesselNaming(toggle: false));
					EditorLogic.fetch.shipNameField.onEndEdit.AddListener(VesselNaming(toggle: true));
				}
				if (EditorLogic.fetch.shipDescriptionField != null)
				{
					EditorLogic.fetch.shipDescriptionField.onSelect.AddListener(VesselNaming(toggle: false));
					EditorLogic.fetch.shipDescriptionField.onEndEdit.AddListener(VesselNaming(toggle: true));
				}
			}
		}

		internal void UpdateMenuNavigationSelectables()
		{
			if (menuNavigation == null)
			{
				return;
			}
			menuNavigation.ResetSelectablesOnly();
			mainSelectables.Clear();
			actiongroupSelectables.Clear();
			for (int i = 0; i < actionGroupList.Objects.Count; i++)
			{
				actiongroupSelectables.Add(actionGroupList.Objects[i].SelectableComponent);
			}
			groupActionsSelectables.Clear();
			for (int j = 0; j < groupActionsList.Objects.Count; j++)
			{
				if (!groupActionsList.Objects[j].gameObject.name.Contains("SeparatorGroupOverrideHeader"))
				{
					groupActionsSelectables.Add(groupActionsList.Objects[j].SelectableComponent);
				}
			}
			selectionSelectables.Clear();
			for (int k = 0; k < partActionList.Objects.Count; k++)
			{
				selectionSelectables.Add(partActionList.Objects[k].SelectableComponent);
			}
			mainSelectables.AddRange(actiongroupSelectables);
			mainSelectables.AddRange(groupActionsSelectables);
			mainSelectables.AddRange(selectionSelectables);
			mainSelectables.Add(additionalActionsToggle);
			menuNavigation.SetSelectableItems(mainSelectables.ToArray(), Navigation.Mode.Vertical, hasText: false, resetNavMode: true);
			SetExplicitNavigation();
		}

		private void SetExplicitNavigation()
		{
			for (int i = 0; i < groupActionsSelectables.Count; i++)
			{
				Navigation navigation = default(Navigation);
				navigation.mode = Navigation.Mode.Explicit;
				if (i - 1 >= 0)
				{
					navigation.selectOnUp = groupActionsSelectables[i - 1];
				}
				if (i + 1 < groupActionsSelectables.Count)
				{
					navigation.selectOnDown = groupActionsSelectables[i + 1];
				}
				if (groupActionsSelectables[i] != null && (bool)groupActionsSelectables[i].GetComponent<EditorActionOverrideGroup>())
				{
					EditorActionOverrideGroup component = groupActionsSelectables[i].GetComponent<EditorActionOverrideGroup>();
					if (component != null)
					{
						component.menuNavigation = menuNavigation;
						Transform transform = groupActionsSelectables[i].transform.Find("Edit Button");
						if (transform != null)
						{
							Selectable component2 = transform.GetComponent<Selectable>();
							if (component2 != null)
							{
								menuNavigation.selectableItems.Add(component2);
								Selectable component3 = transform.GetComponent<Selectable>();
								if (component3 != null)
								{
									navigation.selectOnRight = component3;
								}
							}
						}
					}
				}
				groupActionsSelectables[i].navigation = navigation;
			}
			SetColumnLastItemExplicitNavigation(actiongroupSelectables, additionalActionsToggle);
			SetColumnLastItemExplicitNavigation(groupActionsSelectables, additionalActionsToggle);
			SetColumnLastItemExplicitNavigation(selectionSelectables, additionalActionsToggle);
		}

		private void SetColumnLastItemExplicitNavigation(List<Selectable> selList, Selectable lastSelectable)
		{
			Navigation navigation = default(Navigation);
			navigation.mode = Navigation.Mode.Explicit;
			if (selList.Count > 0)
			{
				if (selList.Count > 1)
				{
					navigation.selectOnUp = selList[selList.Count - 2];
				}
				navigation.selectOnDown = lastSelectable;
				selList[selList.Count - 1].navigation = navigation;
			}
		}

		private void NavigateColumn(ColumnDirection dir)
		{
			if (!(menuNavigation != null) || !menuNavigation.isActiveAndEnabled || !(EventSystem.current != null) || !(EventSystem.current.currentSelectedGameObject != null))
			{
				return;
			}
			int num = 0;
			currentColumnFound = false;
			currentSelectable = EventSystem.current.currentSelectedGameObject.GetComponent<Selectable>();
			if (currentSelectable == null)
			{
				return;
			}
			num = GetCurrentSelectableColumn(currentSelectable);
			if (!currentColumnFound)
			{
				return;
			}
			int num2 = 0;
			switch (dir)
			{
			case ColumnDirection.Left:
				switch (num)
				{
				case 3:
					if (groupActionsSelectables.Count <= 0 && selectionSelectables[num2] != null)
					{
						num2 = Mathf.Clamp(GetCurrentSelectableIndex(currentSelectable, selectionSelectables), 0, actiongroupSelectables.Count - 1);
						SelectNextColumnItem(actiongroupSelectables, num2);
					}
					else
					{
						num2 = Mathf.Clamp(GetCurrentSelectableIndex(currentSelectable, selectionSelectables), 0, groupActionsSelectables.Count - 1);
						SelectNextColumnItem(groupActionsSelectables, num2);
					}
					break;
				case 2:
					num2 = Mathf.Clamp(GetCurrentSelectableIndex(currentSelectable, groupActionsSelectables), 0, actiongroupSelectables.Count - 1);
					SelectNextColumnItem(actiongroupSelectables, num2);
					break;
				}
				break;
			case ColumnDirection.Right:
				switch (num)
				{
				case 1:
					if (flightExitButton != null && groupActionsSelectables.Count <= 0 && selectionSelectables.Count <= 0)
					{
						menuNavigation.SetItemAsFirstSelected(flightExitButton.gameObject);
					}
					else if (groupActionsSelectables.Count <= 0 && selectionSelectables.Count > num2 && selectionSelectables[num2] != null)
					{
						num2 = Mathf.Clamp(GetCurrentSelectableIndex(currentSelectable, actiongroupSelectables), 0, selectionSelectables.Count - 1);
						SelectNextColumnItem(selectionSelectables, num2);
					}
					else
					{
						num2 = Mathf.Clamp(GetCurrentSelectableIndex(currentSelectable, actiongroupSelectables), 0, groupActionsSelectables.Count - 1);
						SelectNextColumnItem(groupActionsSelectables, num2);
					}
					break;
				case 2:
					num2 = Mathf.Clamp(GetCurrentSelectableIndex(currentSelectable, groupActionsSelectables), 0, selectionSelectables.Count - 1);
					SelectNextColumnItem(selectionSelectables, num2);
					break;
				case 3:
					if (flightExitButton != null)
					{
						menuNavigation.SetItemAsFirstSelected(flightExitButton.gameObject);
					}
					break;
				}
				break;
			}
		}

		private void SelectNextColumnItem(List<Selectable> targetList, int index)
		{
			if (targetList.Count > 0 && targetList.Count > index && targetList[index] != null)
			{
				menuNavigation.SetItemAsFirstSelected(targetList[index].gameObject);
			}
		}

		private int GetCurrentSelectableColumn(Selectable item)
		{
			int result = 0;
			currentColumnFound = false;
			if (!currentColumnFound && actiongroupSelectables.Contains(item))
			{
				result = 1;
				currentColumnFound = true;
			}
			if (!currentColumnFound && groupActionsSelectables.Contains(item))
			{
				result = 2;
				currentColumnFound = true;
			}
			if (!currentColumnFound && selectionSelectables.Contains(item))
			{
				result = 3;
				currentColumnFound = true;
			}
			return result;
		}

		private int GetCurrentSelectableIndex(Selectable item, List<Selectable> selectablecolumn = null)
		{
			int result = 0;
			if (selectablecolumn != null)
			{
				result = selectablecolumn.IndexOf(item);
			}
			return result;
		}

		private void CacheCurrentSelectable()
		{
			if (!(menuNavigation != null) || !menuNavigation.isActiveAndEnabled || !(EventSystem.current != null) || !(EventSystem.current.currentSelectedGameObject != null))
			{
				return;
			}
			currentSelectable = EventSystem.current.currentSelectedGameObject.GetComponent<Selectable>();
			if (currentSelectable != null)
			{
				lastSelectedItemColumn = GetCurrentSelectableColumn(currentSelectable);
				switch (lastSelectedItemColumn)
				{
				case 1:
					lastSelectedItemIndex = GetCurrentSelectableIndex(currentSelectable, actiongroupSelectables);
					break;
				case 2:
					lastSelectedItemIndex = GetCurrentSelectableIndex(currentSelectable, groupActionsSelectables);
					break;
				case 3:
					lastSelectedItemIndex = GetCurrentSelectableIndex(currentSelectable, selectionSelectables);
					break;
				}
			}
		}

		private UnityAction<string> VesselNaming(bool toggle)
		{
			return delegate
			{
				SetMenuNavEnabled(toggle);
			};
		}

		private void SetMenuNavEnabled(bool enabled)
		{
			if (menuNavigation != null)
			{
				if (!interfaceActive && enabled)
				{
					enabled = false;
				}
				menuNavigation.enabled = enabled;
			}
		}

		private IEnumerator ToggleMenuNavigationInputLock(bool state)
		{
			yield return null;
			if (menuNavigation != null)
			{
				menuNavigation.SetMenuNavInputLock(state);
			}
		}
	}
}

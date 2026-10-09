using System;
using System.Collections.Generic;
using KSP.UI;
using KSP.UI.Screens;
using KSP.UI.TooltipTypes;
using UnityEngine;
using UnityEngine.SceneManagement;

public class UIPartActionControllerInventory : MonoBehaviour
{
	public static UIPartActionControllerInventory Instance;

	public static bool heldPartIsStack;

	public static int stackSize;

	public static int amountToStore;

	[SerializeField]
	private UIPartActionController pawController;

	protected ModuleInventoryPart currentInventory;

	protected UIPartActionInventorySlot currentInventorySlotClicked;

	protected UIPartActionInventorySlot currentInventorySlotHovered;

	protected UIPartActionInventorySlot currentInventorySlotEmptied;

	[Obsolete("Not in use")]
	public bool currentPartSnapping;

	[Obsolete("Not in use")]
	public bool hoveredPartIsKerbal;

	public bool currentPartIsAttachable;

	protected Part currentCargoPart;

	protected Part exchangedCargoPart;

	private uint currentHoveredInventoryPart;

	public GameObject inventoryTooltipPrefab;

	protected UIPartActionWindow currentPawAutoOpened;

	internal bool currentPawAutoOpening;

	public bool IsCursorOverAnyPAWOrCargoPane;

	[SerializeField]
	private float cargoPartDepthOffset;

	[SerializeField]
	private float partScaleMultiplier;

	[SerializeField]
	private float partScale;

	public bool isHeldPartOutOfActionWindows;

	public bool referenceNextCreatedInventoryPAW;

	public Sprite slot_partItem_normal;

	public Sprite slot_partItem_current;

	[SerializeField]
	private GameObject flight_TooltipPrefab;

	private GameObject currentTooltip;

	private bool currentCargoExists;

	private Vector3 partOriginalScale;

	private List<string> availableCargoParts;

	[HideInInspector]
	public List<string> availableInventoryParts = new List<string>();

	private float currentDistanceFromPart;

	public bool editorPartPickedBlockSfx;

	public bool editorPartDroppedBlockSfx;

	public Shader noAmbientHue;

	public Shader noAmbientHueBumped;

	[SerializeField]
	private Light cargoPartsLight;

	private int cargoInventoriesOpen;

	private bool enableCargoLight;

	private Vector2 heldPartPosition;

	private Vector3 heldPartNewPosition;

	public float partIconDepthDistance = 39f;

	public float iconHeight = 58f;

	public bool isSetAsPart;

	public GameObject InventoryOnlyIconPrefab;

	[HideInInspector]
	public EditorInventoryOnlyIcon CurrentInventoryOnlyIcon;

	[SerializeField]
	private AudioSource audioSource;

	[SerializeField]
	private AudioClip pickupPart;

	[SerializeField]
	private AudioClip droppedPart;

	public ModuleInventoryPart CurrentInventory
	{
		get
		{
			return currentInventory;
		}
		set
		{
			currentInventory = value;
		}
	}

	public UIPartActionInventorySlot CurrentInventorySlotClicked
	{
		get
		{
			return currentInventorySlotClicked;
		}
		set
		{
			currentInventorySlotClicked = value;
		}
	}

	public UIPartActionInventorySlot CurrentInventorySlotHovered
	{
		get
		{
			return currentInventorySlotHovered;
		}
		set
		{
			currentInventorySlotHovered = value;
		}
	}

	public ModuleInventoryPart CurrentInventorySlotHoveredModule => currentInventorySlotHovered.moduleInventoryPart;

	public UIPartActionInventorySlot CurrentInventorySlotEmptied
	{
		get
		{
			return currentInventorySlotEmptied;
		}
		set
		{
			currentInventorySlotEmptied = value;
		}
	}

	public Part CurrentCargoPart
	{
		get
		{
			return currentCargoPart;
		}
		set
		{
			currentCargoPart = value;
			GameEvents.OnInventoryPartOnMouseChanged.Fire(CurrentCargoPart);
		}
	}

	public ModuleCargoPart CurrentModuleCargoPart
	{
		get
		{
			if (currentCargoPart != null)
			{
				return currentCargoPart.FindModuleImplementing<ModuleCargoPart>();
			}
			return null;
		}
	}

	[Obsolete("Not in use")]
	public Part ExchangedCargoPart
	{
		get
		{
			return exchangedCargoPart;
		}
		set
		{
			exchangedCargoPart = value;
		}
	}

	[Obsolete("Not in use")]
	public uint CurrentHoveredInventoryPart => currentHoveredInventoryPart;

	public UIPartActionWindow CurrentPawAutoOpened
	{
		get
		{
			return currentPawAutoOpened;
		}
		set
		{
			currentPawAutoOpened = value;
		}
	}

	private void Awake()
	{
		if (Instance != null)
		{
			Debug.LogError("[UIPartActionControllerInventory]: Instance exists.");
			UnityEngine.Object.Destroy(this);
		}
		else
		{
			Instance = this;
		}
	}

	private void Start()
	{
		if (HighLogic.LoadedSceneIsEditor)
		{
			GameEvents.onEditorPartEvent.Add(OnCargoPartConstructionEvent);
		}
		availableCargoParts = PartLoader.Instance.GetAvailableCargoPartNames();
		availableInventoryParts = PartLoader.Instance.GetAvailableCargoInventoryPartNames();
		currentHoveredInventoryPart = 0u;
		CurrentPawAutoOpened = null;
		partOriginalScale = new Vector3(partScale, partScale, partScale);
		cargoPartsLight.gameObject.SetActive(value: false);
		cargoInventoriesOpen = 0;
		enableCargoLight = false;
		UpdateAudioVolume();
		GameEvents.OnGameSettingsApplied.Add(UpdateAudioVolume);
		GameEvents.onVesselDestroy.Add(OnVesselDestroy);
		GameEvents.OnCameraChange.Add(OnCameraChange);
		GameEvents.OnPartPurchased.Add(OnPartPurchased);
	}

	private void OnDestroy()
	{
		GameEvents.onEditorPartEvent.Remove(OnCargoPartConstructionEvent);
		GameEvents.OnGameSettingsApplied.Remove(UpdateAudioVolume);
		GameEvents.onVesselDestroy.Remove(OnVesselDestroy);
		GameEvents.OnCameraChange.Remove(OnCameraChange);
		GameEvents.OnPartPurchased.Remove(OnPartPurchased);
		DestroyTooltip();
	}

	public bool IsKerbalWithinRange(ModuleInventoryPart inventoryPart)
	{
		if (FlightGlobals.ActiveVessel.isEVA)
		{
			currentDistanceFromPart = Vector3.Distance(FlightGlobals.ActiveVessel.GetWorldPos3D(), inventoryPart.transform.position);
		}
		return currentDistanceFromPart <= GameSettings.EVA_INVENTORY_RANGE;
	}

	private void OnCargoPartConstructionEvent(ConstructionEventType construction, Part p)
	{
		if (!availableCargoParts.Contains(p.partInfo.name))
		{
			return;
		}
		switch (construction)
		{
		case ConstructionEventType.PartOverInventoryGrid:
			if (CurrentInventorySlotClicked != null)
			{
				CurrentInventorySlotClicked.UpdateCurrentSelectedSlot(isCurrent: false);
			}
			break;
		case ConstructionEventType.PartCreated:
			CurrentCargoPart = p;
			break;
		case ConstructionEventType.PartPicked:
			CurrentCargoPart = p;
			break;
		case ConstructionEventType.PartDropped:
		case ConstructionEventType.PartAttached:
		case ConstructionEventType.PartDeleted:
			if (CurrentInventorySlotClicked != null)
			{
				CurrentInventorySlotClicked.UpdateCurrentSelectedSlot(isCurrent: false);
			}
			ResetInventoryCacheValues();
			break;
		}
	}

	public void ResetInventoryCacheValues()
	{
		ResetInventoryCacheValues(destroyCurrentCargoPart: true);
	}

	internal void ResetInventoryCacheValues(bool destroyCurrentCargoPart)
	{
		if (CurrentCargoPart != null && destroyCurrentCargoPart)
		{
			UnityEngine.Object.Destroy(CurrentCargoPart.gameObject);
			if (CurrentInventoryOnlyIcon != null)
			{
				UnityEngine.Object.Destroy(CurrentInventoryOnlyIcon.gameObject);
				CurrentInventoryOnlyIcon = null;
			}
		}
		CurrentInventory = null;
		CurrentInventorySlotClicked = null;
		CurrentCargoPart = null;
		GameEvents.OnInventoryPartOnMouseChanged.Fire(null);
		referenceNextCreatedInventoryPAW = false;
		currentHoveredInventoryPart = 0u;
		if (!(CurrentPawAutoOpened != null))
		{
			return;
		}
		for (int i = 0; i < pawController.windows.Count; i++)
		{
			if (!pawController.windows[i].Hover && pawController.windows[i].part.persistentId == CurrentPawAutoOpened.part.persistentId)
			{
				GameEvents.onPartActionUIDismiss.Fire(pawController.windows[i].part);
				UnityEngine.Object.DestroyImmediate(pawController.windows[i].gameObject);
				pawController.windows.RemoveAt(i);
				break;
			}
		}
		CurrentPawAutoOpened = null;
	}

	public void OnInventoryPartHover(uint partId, Part p)
	{
		for (int i = 0; i < pawController.windows.Count; i++)
		{
			if (pawController.windows[i].part.persistentId == partId)
			{
				return;
			}
		}
		if (currentHoveredInventoryPart != partId)
		{
			currentHoveredInventoryPart = p.persistentId;
			if (Instance.CurrentCargoPart != null && !Instance.CurrentCargoPart.attachRules.StackOrSurfaceAttachable)
			{
				referenceNextCreatedInventoryPAW = true;
				UIPartActionController.Instance.SpawnPartActionWindow(p, overrideSymmetry: true);
			}
		}
	}

	internal void UpdateCursorOverPAWs()
	{
		bool flag = false;
		for (int i = 0; i < pawController.windows.Count; i++)
		{
			flag |= pawController.windows[i].Hover;
		}
		if (HighLogic.LoadedSceneIsFlight)
		{
			flag |= EVAConstructionModeController.Instance.Hover;
		}
		else if (HighLogic.LoadedSceneIsEditor)
		{
			flag |= InventoryPanelController.Instance.Hover;
		}
		IsCursorOverAnyPAWOrCargoPane = flag;
	}

	public void UpdateCurrentCargoPart()
	{
		currentCargoExists = CurrentCargoPart != null;
		if (!(EVAConstructionModeController.Instance != null) || !EVAConstructionModeController.Instance.IsOpen || !(EVAConstructionModeController.Instance.evaEditor != null) || !(EVAConstructionModeController.Instance.evaEditor.SelectedPart != null))
		{
			MoveSelectedPart();
		}
		if (!isSetAsPart)
		{
			UpdatePartIconScale();
		}
		UpdateSelectedPartVisibility();
	}

	private void MoveSelectedPart()
	{
		if (isHeldPartOutOfActionWindows && currentCargoExists)
		{
			CurrentCargoPart.transform.localPosition = dragOverPlane();
		}
	}

	private Vector3 dragOverPlane()
	{
		RectTransformUtility.ScreenPointToLocalPointInRectangle(UIMasterController.Instance.actionCanvas.transform as RectTransform, Input.mousePosition, UIMainCamera.Camera, out heldPartPosition);
		heldPartNewPosition.z = cargoPartDepthOffset;
		heldPartNewPosition.x = heldPartPosition.x;
		heldPartNewPosition.y = heldPartPosition.y;
		return heldPartNewPosition;
	}

	public void ReturnHeldPart()
	{
		if (CurrentInventorySlotClicked != null && CurrentCargoPart != null)
		{
			GameObject gameObject = CurrentCargoPart.gameObject;
			CurrentInventorySlotClicked.ReturnHeldPartToThisSlot();
			if (gameObject != null)
			{
				UnityEngine.Object.Destroy(gameObject);
			}
			if (CurrentInventoryOnlyIcon != null)
			{
				UnityEngine.Object.Destroy(CurrentInventoryOnlyIcon.gameObject);
			}
		}
	}

	private void OnVesselDestroy(Vessel v)
	{
		if (CurrentInventory != null && CurrentInventory.vessel != null && CurrentInventory.vessel.persistentId == v.persistentId)
		{
			ReturnHeldPart();
		}
	}

	public void CreateFlightTooltip(AvailablePart ap)
	{
		if (ap != null)
		{
			ModuleCargoPart component = ap.partPrefab.GetComponent<ModuleCargoPart>();
			if (!(component == null))
			{
				currentTooltip = UnityEngine.Object.Instantiate(flight_TooltipPrefab, UIMasterController.Instance.tooltipCanvas.transform);
				Tooltip_TitleAndText component2 = currentTooltip.GetComponent<Tooltip_TitleAndText>();
				component2.title.text = ap.title;
				component2.label.text = component.GetTooltip();
				Canvas.ForceUpdateCanvases();
				UIMasterController.RepositionTooltip((RectTransform)currentTooltip.transform, Vector2.one);
			}
		}
	}

	public void DestroyTooltip()
	{
		if (currentTooltip != null)
		{
			UnityEngine.Object.Destroy(currentTooltip);
			currentTooltip = null;
		}
	}

	public void ToggleCargoPartsLight(int counter)
	{
		if (!HighLogic.LoadedSceneIsEditor)
		{
			cargoInventoriesOpen += counter;
			enableCargoLight = cargoInventoriesOpen > 0;
			cargoPartsLight.gameObject.SetActive(enableCargoLight);
		}
	}

	public void PlayPartSelectedSFX()
	{
		audioSource.PlayOneShot(pickupPart);
	}

	public void PlayPartDroppedSFX()
	{
		audioSource.PlayOneShot(droppedPart);
	}

	private void UpdateAudioVolume()
	{
		audioSource.volume = GameSettings.UI_VOLUME;
	}

	private void OnCameraChange(CameraManager.CameraMode mode)
	{
		if (mode == CameraManager.CameraMode.IVA || mode == CameraManager.CameraMode.Internal || mode == CameraManager.CameraMode.Map)
		{
			DestroyTooltip();
		}
	}

	private void OnPartPurchased(AvailablePart part)
	{
		availableCargoParts = PartLoader.Instance.GetAvailableCargoPartNames();
		availableInventoryParts = PartLoader.Instance.GetAvailableCargoInventoryPartNames();
	}

	internal void DestroyHeldPartAsIcon()
	{
		if (CurrentInventoryOnlyIcon != null)
		{
			UnityEngine.Object.Destroy(CurrentInventoryOnlyIcon.gameObject);
		}
	}

	public void SetPartAsIcon(Transform parent, Part part)
	{
		part.transform.SetParent(parent);
		part.transform.localPosition = new Vector3(0f, 0f, 0f - partIconDepthDistance);
		float num = 2f;
		num = PartGeometryUtil.MergeBounds(PartGeometryUtil.GetPartRendererBounds(part)).size.magnitude;
		part.transform.localScale = Vector3.one * (iconHeight / num);
		part.transform.rotation = Quaternion.Euler(-15f, 0f, 0f);
		part.transform.Rotate(0f, -30f, 0f);
		U5Util.SetLayerRecursive(part.gameObject, LayerMask.NameToLayer("UIAdditional"));
		part.highlighter.ReinitMaterials();
		part.SetHighlightType(Part.HighlightType.OnMouseOver);
		part.SetHighlight(active: false, recursive: true);
		isSetAsPart = false;
	}

	public void SetIconAsPart()
	{
		if (IsCursorOverAnyPAWOrCargoPane || isSetAsPart || !(UIPartActionController.Instance != null) || !UIPartActionController.Instance.InventoryAndCargoPartExist())
		{
			return;
		}
		if (HighLogic.LoadedSceneIsEditor)
		{
			EditorLogic.fetch.SetIconAsPart(Instance.CurrentCargoPart);
			isSetAsPart = !heldPartIsStack;
		}
		if (!HighLogic.LoadedSceneIsFlight || !(currentCargoPart != null))
		{
			return;
		}
		if (EVAConstructionModeController.Instance != null && EVAConstructionModeController.Instance.IsOpen && EVAConstructionModeController.Instance.evaEditor.SelectedPart != null)
		{
			currentCargoPart.transform.SetParent(null);
			SceneManager.MoveGameObjectToScene(currentCargoPart.gameObject, SceneManager.GetActiveScene());
			currentCargoPart.transform.rotation = Quaternion.identity;
			currentCargoPart.transform.localScale = Vector3.one;
			currentCargoPart.attRotation = Quaternion.identity;
			currentCargoPart.attRotation0 = currentCargoPart.transform.localRotation;
			currentCargoPart.attPos0 = currentCargoPart.transform.localPosition;
			currentCargoPart.gameObject.SetLayerRecursive(0, filterTranslucent: true, 2097152);
			if (currentCargoPart.rb == null)
			{
				currentCargoPart.rb = currentCargoPart.gameObject.AddComponent<Rigidbody>();
			}
			currentCargoPart.rb.isKinematic = true;
			currentCargoPart.rb.useGravity = false;
			currentCargoPart.highlighter.ReinitMaterials();
			currentCargoPart.SetHighlightType(Part.HighlightType.AlwaysOn);
			currentCargoPart.SetHighlight(active: true, recursive: true);
			isSetAsPart = true;
		}
		else
		{
			CurrentCargoPart.transform.SetParent(UIMasterController.Instance.actionCanvas.transform);
			isSetAsPart = true;
		}
	}

	private void UpdatePartIconScale()
	{
		if (currentCargoExists)
		{
			if (!IsCursorOverAnyPAWOrCargoPane && EVAConstructionModeController.Instance != null && !EVAConstructionModeController.Instance.Hover)
			{
				CurrentCargoPart.transform.localScale = partOriginalScale * partScaleMultiplier;
			}
			else
			{
				CurrentCargoPart.transform.localScale = partOriginalScale;
			}
		}
	}

	internal void UpdateSelectedPartVisibility()
	{
		bool flag = false;
		if (CurrentInventoryOnlyIcon != null)
		{
			flag = IsCursorOverAnyPAWOrCargoPane;
			if (HighLogic.LoadedSceneIsFlight)
			{
				flag = true;
				if (EVAConstructionModeController.Instance != null && EVAConstructionModeController.Instance.IsOpen)
				{
					switch (EVAConstructionModeController.Instance.panelMode)
					{
					case EVAConstructionModeController.PanelMode.Cargo:
						flag = true;
						break;
					case EVAConstructionModeController.PanelMode.Construction:
						if (!IsCursorOverAnyPAWOrCargoPane)
						{
							flag = false;
						}
						break;
					}
				}
			}
			if (!flag && CurrentInventoryOnlyIcon != null && stackSize > 1)
			{
				flag = true;
			}
			if (CurrentInventoryOnlyIcon.gameObject.activeInHierarchy != flag)
			{
				CurrentInventoryOnlyIcon.gameObject.SetActive(flag);
			}
		}
		if (!(CurrentCargoPart != null))
		{
			return;
		}
		flag = !IsCursorOverAnyPAWOrCargoPane;
		if (HighLogic.LoadedSceneIsFlight && EVAConstructionModeController.Instance != null)
		{
			if (EVAConstructionModeController.Instance.IsOpen)
			{
				if (EVAConstructionModeController.Instance.panelMode == EVAConstructionModeController.PanelMode.Cargo)
				{
					flag = false;
				}
			}
			else
			{
				flag = false;
			}
		}
		if (flag && CurrentInventoryOnlyIcon != null && stackSize > 1)
		{
			flag = false;
		}
		if (CurrentCargoPart.gameObject.activeInHierarchy == flag)
		{
			return;
		}
		bool num = !CurrentCargoPart.gameObject.activeInHierarchy && flag;
		CurrentCargoPart.gameObject.SetActive(flag);
		if (num)
		{
			for (int i = 0; i < CurrentCargoPart.Modules.Count; i++)
			{
				CurrentCargoPart.Modules[i].OnInventoryModeDisable();
			}
		}
	}

	[Obsolete("Please use UIPartActionControllerInventory.Instance.CreatePartFromInventory(ProtoPartSnapshot protoPartSnapshot)")]
	public Part CreatePartFromInventory(AvailablePart partInfo)
	{
		Part part = UnityEngine.Object.Instantiate(partInfo.partPrefab);
		ConfigureInventoryPart(part);
		if (HighLogic.LoadedSceneIsFlight)
		{
			part.OnInventoryModeDisable();
			for (int i = 0; i < part.Modules.Count; i++)
			{
				part.Modules[i].enabled = false;
			}
			part.enabled = false;
		}
		return part;
	}

	public Part CreatePartFromInventory(ProtoPartSnapshot protoPartSnapshot)
	{
		Part part = protoPartSnapshot.CreatePart();
		ConfigureInventoryPart(part);
		if (HighLogic.LoadedSceneIsFlight)
		{
			part.OnInventoryModeDisable();
			if (HighLogic.LoadedSceneIsFlight)
			{
				part.OnPartCreatedFomInventory(currentInventory);
				for (int i = 0; i < part.Modules.Count; i++)
				{
					part.Modules[i].enabled = false;
					part.Modules[i].OnPartCreatedFomInventory(currentInventory);
				}
			}
			part.enabled = false;
		}
		return part;
	}

	private void ConfigureInventoryPart(Part invPart)
	{
		if (HighLogic.LoadedSceneIsFlight)
		{
			invPart.State = PartStates.CARGO;
			invPart.ResumeState = PartStates.CARGO;
		}
		if (HighLogic.LoadedSceneIsFlight || !invPart.attachRules.StackOrSurfaceAttachable)
		{
			invPart.SetupHighlighter();
			invPart.RefreshHighlighter();
			invPart.SetHighlightType(Part.HighlightType.AlwaysOn);
			invPart.SetHighlight(active: true, recursive: true);
		}
		for (int i = 0; i < invPart.Modules.Count; i++)
		{
			invPart.Modules[i].OnInventoryModeDisable();
			invPart.Modules[i].enabled = false;
		}
		invPart.gameObject.SetActive(value: true);
		invPart.name = invPart.name;
		invPart.persistentId = FlightGlobals.CheckPartpersistentId(invPart.persistentId, invPart, removeOldId: false, addNewId: true);
	}
}

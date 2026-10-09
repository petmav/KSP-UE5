using KSP.UI.Screens.Flight.Dialogs;
using KSP.UI.TooltipTypes;
using UnityEngine;
using UnityEngine.EventSystems;

public class CrewHatchController : MonoBehaviour
{
	[SerializeField]
	private Tooltip_Text crewHatchTooltip;

	public static CrewHatchController fetch;

	private bool interfaceEnabled;

	private RaycastHit rayHit;

	private Part hoveredPart;

	private CrewHatchDialog crewDialog;

	private CrewTransfer crewTransfer;

	public Vector2 anchorOffset = new Vector2(0.5f, -0.5f);

	public bool overrideTransfer;

	private Tooltip_Text CrewHatchTooltip
	{
		get
		{
			if (crewHatchTooltip == null)
			{
				GameObject prefab = AssetBase.GetPrefab("Tooltip_CrewHatch");
				if (prefab != null)
				{
					crewHatchTooltip = Object.Instantiate(prefab).GetComponent<Tooltip_Text>();
					crewHatchTooltip.transform.SetParent(DialogCanvasUtil.DialogCanvasRect, worldPositionStays: false);
				}
			}
			return crewHatchTooltip;
		}
	}

	public CrewHatchDialog CrewDialog
	{
		get
		{
			return crewDialog;
		}
		set
		{
			crewDialog = value;
		}
	}

	public bool Active
	{
		get
		{
			if (!(crewDialog != null))
			{
				return crewTransfer != null;
			}
			return true;
		}
	}

	private void Awake()
	{
		if (fetch != null)
		{
			Debug.LogError("[CrewHatchController]: Attempting to create another instance of singleton", this);
			Debug.LogError("[CrewHatchController]: An instance of this component already exists!", fetch);
		}
		fetch = this;
		GameEvents.onVesselChange.Add(OnVesselSwitch);
		GameEvents.onVesselSituationChange.Add(onVesselSitChange);
		GameEvents.onGamePause.Add(DespawnUIs);
		GameEvents.onGameUnpause.Add(DespawnUIs);
		interfaceEnabled = true;
	}

	private void OnDestroy()
	{
		GameEvents.onGameUnpause.Remove(DespawnUIs);
		GameEvents.onGamePause.Remove(DespawnUIs);
		GameEvents.onVesselChange.Remove(OnVesselSwitch);
		GameEvents.onVesselSituationChange.Remove(onVesselSitChange);
		DespawnUIs();
		if (fetch != null && fetch == this)
		{
			fetch = null;
		}
	}

	private void LateUpdate()
	{
		if (FlightDriver.Pause)
		{
			return;
		}
		if (crewDialog == null && crewTransfer == null && !EventSystem.current.IsPointerOverGameObject() && Cursor.lockState != CursorLockMode.Locked && interfaceEnabled)
		{
			hoveredPart = null;
			if (Physics.Raycast(FlightCamera.fetch.mainCamera.ScreenPointToRay(Input.mousePosition), out rayHit, 300f, 2097152) && rayHit.collider.gameObject.CompareTag("Airlock"))
			{
				hoveredPart = FlightGlobals.GetPartUpwardsCached(rayHit.collider.gameObject);
			}
			if (hoveredPart != null)
			{
				ShowTooltip();
				TooltipUpdate(hoveredPart);
			}
			else
			{
				HideTooltip();
			}
		}
		else
		{
			HideTooltip();
		}
	}

	protected void ShowTooltip()
	{
		if (!CrewHatchTooltip.gameObject.activeSelf)
		{
			CrewHatchTooltip.gameObject.SetActive(value: true);
		}
	}

	protected void TooltipUpdate(Part hp)
	{
		CrewHatchTooltip.RectTransform.localPosition = GetMouseUiPos();
		if (Mouse.Left.GetClick())
		{
			SpawnCrewDialog(hp, showEVA: true, showTransfer: true);
		}
	}

	protected void HideTooltip()
	{
		if (CrewHatchTooltip != null && CrewHatchTooltip.gameObject.activeSelf)
		{
			CrewHatchTooltip.gameObject.SetActive(value: false);
		}
	}

	protected Vector2 GetMouseUiPos()
	{
		bool zPositive;
		return CanvasUtil.AnchorOffset(CanvasUtil.ScreenToUISpacePos(Input.mousePosition, DialogCanvasUtil.DialogCanvasRect, out zPositive), CrewHatchTooltip.RectTransform, anchorOffset);
	}

	public void SpawnCrewDialog(Part part, bool showEVA, bool showTransfer)
	{
		crewDialog = CrewHatchDialog.Spawn(part, showEVA, showTransfer, OnEVABtn, OnTransferBtn, OnCrewDialogDismiss);
		crewDialog.transform.localPosition = GetMouseUiPos();
	}

	protected void OnEVABtn(ProtoCrewMember crew)
	{
		bool evaUnlocked = GameVariables.Instance.UnlockedEVA(ScenarioUpgradeableFacilities.GetFacilityLevel(SpaceCenterFacility.AstronautComplex));
		if ((evaUnlocked = GameVariables.Instance.EVAIsPossible(evaUnlocked, FlightGlobals.ActiveVessel)) && crew.type != ProtoCrewMember.KerbalType.Tourist && !crew.inactive)
		{
			Transform transform = null;
			transform = ((!(rayHit.collider != null)) ? crewDialog.Part.airlock : rayHit.collider.transform);
			if (transform != null)
			{
				FlightEVA.fetch.spawnEVA(crew, crewDialog.Part, transform);
				DismissDialog();
			}
			else
			{
				ScreenMessages.PostScreenMessage(GameVariables.Instance.GetEVALockedReason(FlightGlobals.ActiveVessel, crew), 5f, ScreenMessageStyle.UPPER_CENTER);
			}
		}
		else
		{
			ScreenMessages.PostScreenMessage(GameVariables.Instance.GetEVALockedReason(FlightGlobals.ActiveVessel, crew), 5f, ScreenMessageStyle.UPPER_CENTER);
		}
	}

	protected void OnTransferBtn(ProtoCrewMember crew)
	{
		overrideTransfer = false;
		GameEvents.onAttemptTransfer.Fire(crew, crewDialog.Part, this);
		if (!overrideTransfer)
		{
			crewTransfer = CrewTransfer.Create(crewDialog.Part, crew, OnCrewTransferDone);
		}
		DismissDialog();
	}

	protected void DismissDialog()
	{
		if (crewDialog != null)
		{
			crewDialog.Terminate();
		}
	}

	protected void OnCrewDialogDismiss()
	{
	}

	protected void DespawnUIs()
	{
		DismissDialog();
		HideTooltip();
	}

	protected void OnVesselSwitch(Vessel v)
	{
		DespawnUIs();
	}

	private void onVesselSitChange(GameEvents.HostedFromToAction<Vessel, Vessel.Situations> evt)
	{
		if (evt.host == FlightGlobals.ActiveVessel)
		{
			DismissDialog();
		}
	}

	public void OnCrewTransferDone(PartItemTransfer.DismissAction dma, Part p)
	{
		crewTransfer = null;
		DismissDialog();
	}

	public void EnableInterface()
	{
		DismissDialog();
		interfaceEnabled = true;
	}

	public void DisableInterface()
	{
		DismissDialog();
		interfaceEnabled = false;
	}

	public void ShowHatchTooltip(bool show)
	{
		interfaceEnabled = show;
	}
}

using System;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class SuitButton : MonoBehaviour, IPointerEnterHandler, IEventSystemHandler, IPointerExitHandler
{
	public enum EVAHelmetNeckRingState
	{
		HelmetNeckRingOn,
		OnlyNeckRingOn,
		HelmetNeckRingOff
	}

	public int buttonIndex;

	[SerializeField]
	private Button button;

	[SerializeField]
	private Button suitLightPickerButton;

	[SerializeField]
	private SuitLightColorPicker suitLightColorPicker;

	public RawImage thumbImg;

	[SerializeField]
	private Image thumbMask;

	private KerbalPreview kerbalPreview;

	private ProtoCrewMember.KerbalSuit kerbalSuit;

	[SerializeField]
	private PreviewPanel previewPanel;

	[SerializeField]
	private Image previewPanelImg;

	[SerializeField]
	private Button helmetNeckRingButton;

	[SerializeField]
	private Sprite[] helmetNeckRingState;

	public ComboSelector comboSelector;

	[SerializeField]
	private TextMeshProUGUI comboName;

	private int helmetStates;

	public EVAHelmetNeckRingState evaHelmetNeckRingState;

	public int helmetNeckRingIndex;

	private float previewRotationSpeed = 0.02f;

	private float previewRotation;

	private Quaternion startRot;

	public SuitLightColorPicker SuitLightColorPicker => suitLightColorPicker;

	public KerbalPreview KerbalPreview
	{
		get
		{
			return kerbalPreview;
		}
		set
		{
			kerbalPreview = value;
		}
	}

	public ProtoCrewMember.KerbalSuit KerbalSuit
	{
		get
		{
			return kerbalSuit;
		}
		set
		{
			kerbalSuit = value;
		}
	}

	public Button HelmetNeckRingButton => helmetNeckRingButton;

	public Sprite[] HelmetNeckRingState => helmetNeckRingState;

	public TextMeshProUGUI ComboName => comboName;

	public int HelmetStates => helmetStates;

	private void Start()
	{
		button.onClick.AddListener(OnButtonClicked);
		suitLightPickerButton.onClick.AddListener(OpenSuitLightPicker);
		helmetNeckRingButton.onClick.AddListener(OnHelmetNeckringSelection);
		if (kerbalSuit == ProtoCrewMember.KerbalSuit.Future)
		{
			suitLightPickerButton.gameObject.SetActive(value: true);
		}
		if (buttonIndex != 0)
		{
			comboName.gameObject.SetActive(value: false);
		}
		helmetStates = Enum.GetValues(typeof(EVAHelmetNeckRingState)).Length;
		startRot = KerbalPreview.transform.rotation;
	}

	private void OnDestroy()
	{
		button.onClick.RemoveListener(OnButtonClicked);
		suitLightPickerButton.onClick.RemoveListener(OpenSuitLightPicker);
		helmetNeckRingButton.onClick.RemoveListener(OnHelmetNeckringSelection);
	}

	private void Update()
	{
		RotatePreview();
	}

	private void RotatePreview()
	{
		if (previewPanel.previewHovered)
		{
			previewRotation += 60f * previewRotationSpeed;
			KerbalPreview.transform.localRotation = startRot * Quaternion.AngleAxis(0f - previewRotation, Vector3.up);
		}
		else
		{
			KerbalPreview.transform.localRotation = Quaternion.Lerp(KerbalPreview.transform.rotation, Quaternion.identity, previewRotationSpeed * 2f);
			previewRotation = 0f;
		}
		previewPanelImg.enabled = !suitLightColorPicker.gameObject.activeSelf;
	}

	public void OnButtonClicked()
	{
		GameEvents.onSuitComboSelection.Fire(comboSelector, comboSelector.comboList[comboSelector.fieldValue].comboId, comboSelector.fieldValue);
	}

	public void OnPointerEnter(PointerEventData eventData)
	{
	}

	public void OnPointerExit(PointerEventData eventData)
	{
	}

	private void OpenSuitLightPicker()
	{
		GameEvents.onClickSuitLightButton.Fire(SuitLightColorPicker);
		GameEvents.onSuitComboSelection.Fire(comboSelector, comboSelector.comboList[comboSelector.fieldValue].comboId, comboSelector.fieldValue);
	}

	private void OnHelmetNeckringSelection()
	{
		GameEvents.onClickHelmetNeckringButton.Fire(this);
		GameEvents.onSuitComboSelection.Fire(comboSelector, comboSelector.comboList[comboSelector.fieldValue].comboId, comboSelector.fieldValue);
	}

	public void GenerateButtonName(string kerbalName, string suitType)
	{
		base.name = kerbalName + suitType;
	}

	public void Select()
	{
		thumbMask.color = button.colors.selectedColor;
	}

	public void Reset()
	{
		thumbMask.color = button.colors.normalColor;
	}
}

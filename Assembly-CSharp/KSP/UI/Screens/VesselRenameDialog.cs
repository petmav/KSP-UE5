using Expansions;
using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public class VesselRenameDialog : MonoBehaviour
	{
		[SerializeField]
		private TMP_InputField nameField;

		private PointerClickHandler nameFieldClickHandler;

		private TypeButton selectedToggle;

		[SerializeField]
		private ToggleGroup typeIconsGroup;

		[SerializeField]
		private TypeButton toggleShip;

		[SerializeField]
		private TypeButton toggleLander;

		[SerializeField]
		private TypeButton toggleRover;

		[SerializeField]
		private TypeButton toggleStation;

		[SerializeField]
		private TypeButton toggleProbe;

		[SerializeField]
		private TypeButton toggleBase;

		[SerializeField]
		private TypeButton toggleSpaceObj;

		[SerializeField]
		private TypeButton toggleDebris;

		[SerializeField]
		private TypeButton toggleAircraft;

		[SerializeField]
		private TypeButton toggleCommunicationsRelay;

		[SerializeField]
		private TypeButton toggleDeployedScience;

		[SerializeField]
		private TMP_Text title;

		[SerializeField]
		private GameObject NamePriorityControls;

		[SerializeField]
		private TMP_Text priorityValue;

		[SerializeField]
		private Slider prioritySlider;

		private bool adjustingViaPart;

		[SerializeField]
		private Button buttonAccept;

		[SerializeField]
		private Button buttonCancel;

		[SerializeField]
		private Button buttonRemove;

		private VesselType vesselType;

		private string vesselName = string.Empty;

		private bool hasValidName;

		[SerializeField]
		private bool allowTypeChange;

		private VesselType lowestType;

		private Callback<string, VesselType> onAccept;

		private Callback onDismiss;

		private Callback onRemove;

		private Callback<string, VesselType, int> onAcceptPart;

		public static VesselRenameDialog Spawn(Vessel v, Callback<string, VesselType> onAccept, Callback onDismiss, bool allowTypeChange, VesselType lowestType)
		{
			VesselRenameDialog component = Object.Instantiate(AssetBase.GetPrefab("VesselRenameDialog")).GetComponent<VesselRenameDialog>();
			component.title.text = Localizer.Format("#autoLOC_900678");
			component.NamePriorityControls.SetActive(value: false);
			component.adjustingViaPart = false;
			component.buttonRemove.gameObject.SetActive(value: false);
			component.transform.SetParent(UIMasterController.Instance.dialogCanvas.transform, worldPositionStays: false);
			component.vesselName = v.vesselName;
			component.vesselType = v.vesselType;
			component.allowTypeChange = allowTypeChange;
			component.lowestType = lowestType;
			component.onAccept = onAccept;
			component.onDismiss = onDismiss;
			return component;
		}

		public static VesselRenameDialog SpawnNameFromPart(Part p, Callback<string, VesselType, int> onAccept, Callback onDismiss, Callback onRemove, bool allowTypeChange, VesselType lowestType)
		{
			VesselRenameDialog component = Object.Instantiate(AssetBase.GetPrefab("VesselRenameDialog")).GetComponent<VesselRenameDialog>();
			component.title.text = Localizer.Format("#autoLOC_8003141", p.partInfo.title);
			component.NamePriorityControls.SetActive(value: true);
			component.adjustingViaPart = true;
			component.prioritySlider.maxValue = GameSettings.VESSEL_NAMING_PRIORTY_LEVEL_MAX;
			component.transform.SetParent(UIMasterController.Instance.dialogCanvas.transform, worldPositionStays: false);
			if (p.vesselNaming != null)
			{
				component.vesselName = p.vesselNaming.vesselName;
				component.vesselType = p.vesselNaming.vesselType;
				component.prioritySlider.value = p.vesselNaming.namingPriority;
				component.buttonRemove.gameObject.SetActive(value: true);
			}
			else
			{
				component.prioritySlider.value = GameSettings.VESSEL_NAMING_PRIORTY_LEVEL_DEFAULT;
				if (HighLogic.LoadedSceneIsEditor)
				{
					component.vesselName = EditorLogic.fetch.ship.shipName;
					component.vesselType = p.vesselType;
				}
				else if (p.vessel != null)
				{
					component.vesselName = p.vessel.vesselName;
					component.vesselType = p.vessel.vesselType;
				}
				component.buttonRemove.gameObject.SetActive(value: false);
			}
			component.priorityValue.text = component.prioritySlider.value.ToString();
			component.allowTypeChange = allowTypeChange;
			component.lowestType = lowestType;
			component.onAcceptPart = onAccept;
			component.onDismiss = onDismiss;
			component.onRemove = onRemove;
			return component;
		}

		protected void Start()
		{
			if (allowTypeChange)
			{
				ToggleSetup(toggleBase, delegate(bool b)
				{
					OnToggle(toggleBase, b);
				});
				ToggleSetup(toggleLander, delegate(bool b)
				{
					OnToggle(toggleLander, b);
				});
				ToggleSetup(toggleProbe, delegate(bool b)
				{
					OnToggle(toggleProbe, b);
				});
				ToggleSetup(toggleRover, delegate(bool b)
				{
					OnToggle(toggleRover, b);
				});
				ToggleSetup(toggleShip, delegate(bool b)
				{
					OnToggle(toggleShip, b);
				});
				ToggleSetup(toggleStation, delegate(bool b)
				{
					OnToggle(toggleStation, b);
				});
				ToggleSetup(toggleAircraft, delegate(bool b)
				{
					OnToggle(toggleAircraft, b);
				});
				ToggleSetup(toggleCommunicationsRelay, delegate(bool b)
				{
					OnToggle(toggleCommunicationsRelay, b);
				});
				if (ExpansionsLoader.IsExpansionInstalled("Serenity"))
				{
					ToggleSetup(toggleDeployedScience, delegate(bool b)
					{
						OnToggle(toggleDeployedScience, b);
					});
				}
				else
				{
					toggleDeployedScience.gameObject.SetActive(value: false);
				}
				if (lowestType == VesselType.SpaceObject)
				{
					toggleDebris.gameObject.SetActive(value: false);
					toggleSpaceObj.gameObject.SetActive(value: true);
					ToggleSetup(toggleSpaceObj, delegate(bool b)
					{
						OnToggle(toggleSpaceObj, b);
					});
				}
				else
				{
					toggleDebris.gameObject.SetActive(value: true);
					toggleSpaceObj.gameObject.SetActive(value: false);
					ToggleSetup(toggleDebris, delegate(bool b)
					{
						OnToggle(toggleDebris, b);
					});
				}
			}
			else
			{
				typeIconsGroup.gameObject.SetActive(value: false);
			}
			nameField.text = vesselName;
			nameField.onValueChanged.AddListener(OnNameFieldModified);
			nameField.onEndEdit.AddListener(OnNameFieldEndEdit);
			nameFieldClickHandler = nameField.GetComponent<PointerClickHandler>();
			nameFieldClickHandler.onPointerClick.AddListener(OnNameFieldSelected);
			buttonAccept.onClick.AddListener(OnButtonAccept);
			buttonAccept.interactable = Vessel.IsValidVesselName(vesselName);
			buttonCancel.onClick.AddListener(OnButtonDismiss);
			buttonRemove.onClick.AddListener(OnButtonRemove);
			prioritySlider.onValueChanged.AddListener(OnPriorityChanged);
		}

		private void OnPriorityChanged(float newValue)
		{
			priorityValue.text = newValue.ToString();
		}

		public void Terminate()
		{
			Object.Destroy(base.gameObject);
		}

		protected void ToggleSetup(TypeButton t, UnityAction<bool> onValueChangedCallback)
		{
			typeIconsGroup.RegisterToggle(t.toggle);
			t.toggle.group = typeIconsGroup;
			t.toggle.onValueChanged.AddListener(onValueChangedCallback);
			if (t.type == vesselType)
			{
				t.Select();
				selectedToggle = t;
			}
		}

		protected void OnToggle(TypeButton t, bool b)
		{
			if (b && t != selectedToggle)
			{
				if (selectedToggle != null)
				{
					selectedToggle.Deselect();
				}
				vesselType = t.type;
				selectedToggle = t;
				t.Select();
			}
		}

		protected void OnNameFieldModified(string newName)
		{
			hasValidName = Vessel.IsValidVesselName(newName);
			if (hasValidName)
			{
				vesselName = newName;
				buttonAccept.interactable = true;
			}
			else
			{
				buttonAccept.interactable = false;
			}
		}

		protected void OnNameFieldEndEdit(string s)
		{
			InputLockManager.RemoveControlLock("VesselRenameDialogTextInput");
		}

		protected void OnNameFieldSelected(PointerEventData eventData)
		{
			InputLockManager.SetControlLock(ControlTypes.KEYBOARDINPUT, "VesselRenameDialogTextInput");
		}

		protected void OnButtonAccept()
		{
			if (!adjustingViaPart)
			{
				onAccept(vesselName, vesselType);
			}
			else
			{
				onAcceptPart(vesselName, vesselType, (int)prioritySlider.value);
			}
			Terminate();
		}

		protected void OnButtonDismiss()
		{
			onDismiss();
			Terminate();
		}

		protected void OnButtonRemove()
		{
			onRemove();
			Terminate();
		}

		protected void Update()
		{
			if (Input.GetKeyDown(KeyCode.Return) && !nameField.isFocused && hasValidName)
			{
				OnButtonAccept();
			}
			if (Input.GetKeyDown(KeyCode.Escape) && !nameField.isFocused)
			{
				OnButtonDismiss();
			}
		}
	}
}

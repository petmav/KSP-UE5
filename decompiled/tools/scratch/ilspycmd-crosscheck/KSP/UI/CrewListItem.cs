using System;
using Expansions;
using Expansions.Missions;
using Expansions.Serenity;
using KSP.UI.TooltipTypes;
using TMPro;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.UI;

namespace KSP.UI
{
	public class CrewListItem : MonoBehaviour
	{
		[Serializable]
		public class ClickEvent<ButtonTypes, CrewListItem> : UnityEvent<ButtonTypes, CrewListItem>
		{
		}

		public enum ButtonTypes
		{
			X,
			V,
			X2
		}

		public enum KerbalTypes
		{
			AVAILABLE,
			BADASS,
			EVA,
			RECRUIT,
			TOURIST
		}

		private bool mouseoverEnabled = true;

		public UIStateButton button;

		[SerializeField]
		private Button coatHangerButton;

		[SerializeField]
		private Button kerbalIconButton;

		public TextMeshProUGUI kerbalName;

		[SerializeField]
		public RawImage kerbalSprite;

		public TextMeshProUGUI xp_trait;

		[SerializeField]
		private Slider xp_slider;

		[SerializeField]
		private UIStateImage xp_levels;

		[SerializeField]
		private Slider slider_courage;

		[SerializeField]
		private Slider slider_stupidity;

		[SerializeField]
		private TextMeshProUGUI label;

		[SerializeField]
		private TooltipController_CrewAC tooltipController;

		[SerializeField]
		private UIHoverPanel hoverPanel;

		private ProtoCrewMember crew;

		private bool setup;

		private bool over;

		public bool isEmpty;

		public uint pUid;

		private KerbalTypes lastType;

		public ClickEvent<ButtonTypes, CrewListItem> onClick = new ClickEvent<ButtonTypes, CrewListItem>();

		private SuitCombos suitCombos;

		public bool MouseoverEnabled
		{
			get
			{
				return mouseoverEnabled;
			}
			set
			{
				mouseoverEnabled = value;
				if (hoverPanel != null)
				{
					if (value)
					{
						hoverPanel.EnableHover();
					}
					else
					{
						hoverPanel.DisableHover();
					}
				}
			}
		}

		private void Awake()
		{
			suitCombos = GameDatabase.Instance.GetComponent<SuitCombos>();
			if (button != null)
			{
				button.onClick.AddListener(delegate
				{
					onClick.Invoke((!(button.currentState == "X")) ? ButtonTypes.V : ButtonTypes.X, this);
				});
				if (coatHangerButton != null)
				{
					coatHangerButton.onClick.AddListener(OpenHelmetSuitPickerWindow);
				}
				if (kerbalIconButton != null)
				{
					kerbalIconButton.onClick.AddListener(OpenHelmetSuitPickerWindow);
				}
			}
		}

		private void OpenHelmetSuitPickerWindow()
		{
			if (crew.type != ProtoCrewMember.KerbalType.Applicant && crew.rosterStatus != ProtoCrewMember.RosterStatus.Dead && crew.rosterStatus != ProtoCrewMember.RosterStatus.Missing)
			{
				suitCombos.helmetSuitPickerWindow.gameObject.SetActive(value: true);
				suitCombos.helmetSuitPickerWindow.SetupSuitTypeButtons(this, crew, lastType);
				suitCombos.helmetSuitPickerWindow.SetupWindowTransform(suitCombos.helmetSuitPickerWindow.GetComponent<RectTransform>());
			}
		}

		public void SetTooltip(ProtoCrewMember crew)
		{
			if (tooltipController != null)
			{
				tooltipController.SetTooltip(crew);
			}
			else
			{
				Debug.LogError("[CrewListItem] No tooltip serialized.");
			}
		}

		public void SetName(string name)
		{
			kerbalName.text = name;
		}

		public string GetName()
		{
			return kerbalName.text;
		}

		public void SetLabel(string label)
		{
			if (this.label != null)
			{
				this.label.text = label;
			}
		}

		public void SetXP(ProtoCrewMember pcm)
		{
			if (pcm.experienceTrait == null)
			{
				KerbalRoster.SetExperienceTrait(pcm);
			}
			if (xp_trait != null)
			{
				xp_trait.text = pcm.experienceTrait.Title;
			}
			if (xp_levels != null)
			{
				xp_levels.SetState(pcm.experienceLevel);
			}
			if (xp_slider != null)
			{
				xp_slider.value = pcm.ExperienceLevelDelta;
			}
		}

		public void SetXP(string trait, float xp, int level)
		{
			if (xp_trait != null)
			{
				xp_trait.text = trait;
			}
			if (xp_levels != null)
			{
				xp_levels.SetState(level);
			}
			if (xp_slider != null)
			{
				xp_slider.value = xp;
			}
		}

		public void SetButton(ButtonTypes type)
		{
			switch (type)
			{
			case ButtonTypes.X:
				button.SetState(0);
				break;
			case ButtonTypes.V:
				button.SetState(1);
				break;
			case ButtonTypes.X2:
				button.SetState(2);
				break;
			}
		}

		public void SetButtonEnabled(bool state, string disabledReasonTitle = "", string disabledReasonCaption = "")
		{
			if (state)
			{
				MouseoverEnabled = true;
				tooltipController.SetTooltip(crew);
			}
			else
			{
				MouseoverEnabled = false;
				tooltipController.SetTooltip(crew, disabledReasonTitle, disabledReasonCaption);
			}
		}

		public void SetKerbal(ProtoCrewMember crew, KerbalTypes type)
		{
			lastType = type;
			Texture texture = null;
			if (crew.SpritePath == null)
			{
				string kerbalIconSuitPath = crew.GetKerbalIconSuitPath();
				texture = ((kerbalIconSuitPath.Contains("vintage") && ExpansionsLoader.IsExpansionInstalled("MakingHistory")) ? MissionsUtils.METexture(kerbalIconSuitPath + ".tif") : ((!kerbalIconSuitPath.Contains("future") || !ExpansionsLoader.IsExpansionInstalled("Serenity")) ? AssetBase.GetTexture(kerbalIconSuitPath) : SerenityUtils.SerenityTexture(kerbalIconSuitPath + ".tif")));
			}
			else
			{
				texture = GameDatabase.Instance.GetTexture(crew.SpritePath, asNormalMap: false);
			}
			if (texture != null)
			{
				kerbalSprite.texture = texture;
			}
			else
			{
				Debug.LogWarning("Custom sprite not found, assigning default.");
				crew.SpritePath = null;
				kerbalSprite.texture = AssetBase.GetTexture("kerbalicon_suit");
			}
			SetupSuitButtons(crew);
		}

		public void SetKerbalAsApplicableType(ProtoCrewMember crew)
		{
			if (crew.veteran)
			{
				SetKerbal(crew, KerbalTypes.BADASS);
			}
			else if (crew.type == ProtoCrewMember.KerbalType.Tourist)
			{
				SetKerbal(crew, KerbalTypes.TOURIST);
			}
			else
			{
				SetKerbal(crew, KerbalTypes.AVAILABLE);
			}
		}

		public void AddButtonInputDelegate(UnityAction<ButtonTypes, CrewListItem> del)
		{
			onClick.AddListener(del);
		}

		public void SetStats(ProtoCrewMember pcm)
		{
			SetStats(pcm.courage, pcm.stupidity);
		}

		public void SetStats(float courage, float stupidity)
		{
			if (slider_courage != null)
			{
				slider_courage.value = courage;
			}
			if (slider_stupidity != null)
			{
				slider_stupidity.value = stupidity;
			}
		}

		public float GetCourage()
		{
			return slider_courage.value;
		}

		public float GetStupidity()
		{
			return slider_stupidity.value;
		}

		public void SetCrewRef(ProtoCrewMember crewRef)
		{
			crew = crewRef;
		}

		private void SetupSuitButtons(ProtoCrewMember crew)
		{
			if (crew.type == ProtoCrewMember.KerbalType.Applicant)
			{
				if (coatHangerButton != null)
				{
					coatHangerButton.gameObject.SetActive(value: false);
				}
				if (kerbalIconButton != null)
				{
					kerbalIconButton.transition = Selectable.Transition.None;
					kerbalIconButton.interactable = false;
				}
			}
			else
			{
				if (coatHangerButton != null)
				{
					coatHangerButton.gameObject.SetActive(value: true);
				}
				if (kerbalIconButton != null)
				{
					kerbalIconButton.transition = Selectable.Transition.ColorTint;
					kerbalIconButton.interactable = true;
				}
			}
		}

		public ProtoCrewMember GetCrewRef()
		{
			return crew;
		}
	}
}

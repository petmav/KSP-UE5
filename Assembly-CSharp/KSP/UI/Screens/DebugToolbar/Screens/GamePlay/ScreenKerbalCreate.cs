using System;
using System.Collections;
using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar.Screens.GamePlay
{
	public class ScreenKerbalCreate : MonoBehaviour
	{
		public TMP_InputField nameField;

		public Button randomButton;

		public Toggle maleGender;

		public Toggle femaleGender;

		public Toggle pilotRole;

		public Toggle scientistRole;

		public Toggle engineerRole;

		public Toggle touristRole;

		public Slider experienceSlider;

		public TextMeshProUGUI experienceSliderText;

		public Slider courageSlider;

		public Slider stupiditySlider;

		public Toggle veteranToggle;

		public Toggle badassToggle;

		public Button submitButton;

		public TextMeshProUGUI submitButtonText;

		private string currentName;

		private ProtoCrewMember.Gender currentGender;

		private string currentRole;

		private int experienceLevel;

		private float courageLevel;

		private float stupidityLevel;

		private bool isVeteran;

		private bool isBadass;

		private string lastName;

		private string levelString;

		private KSPRandom generator;

		private void Start()
		{
			InitializeLocals();
			RefreshControls();
			AddListeners();
			CheckForErrors();
			GameEvents.onKerbalAdded.Add(KerbalsModified);
			GameEvents.onKerbalRemoved.Add(KerbalsModified);
			GameEvents.OnCrewmemberHired.Add(KerbalsModified);
			GameEvents.OnCrewmemberSacked.Add(KerbalsModified);
			GameEvents.OnCrewmemberLeftForDead.Add(KerbalsModified);
			GameEvents.onGameStatePostLoad.Add(GameLoaded);
		}

		private void OnDestroy()
		{
			GameEvents.onKerbalAdded.Remove(KerbalsModified);
			GameEvents.onKerbalRemoved.Remove(KerbalsModified);
			GameEvents.OnCrewmemberHired.Remove(KerbalsModified);
			GameEvents.OnCrewmemberSacked.Remove(KerbalsModified);
			GameEvents.OnCrewmemberLeftForDead.Remove(KerbalsModified);
			GameEvents.onGameStatePostLoad.Remove(GameLoaded);
		}

		private void InitializeLocals()
		{
			generator = new KSPRandom(Environment.TickCount ^ Guid.NewGuid().GetHashCode());
			currentGender = ((generator.Next(2) > 0) ? ProtoCrewMember.Gender.Female : ProtoCrewMember.Gender.Male);
			currentName = CrewGenerator.GetRandomName(currentGender, generator);
			switch (generator.Next(3))
			{
			default:
				currentRole = KerbalRoster.pilotTrait;
				break;
			case 1:
				currentRole = KerbalRoster.engineerTrait;
				break;
			case 0:
				currentRole = KerbalRoster.scientistTrait;
				break;
			}
			experienceLevel = generator.Next(0, 6);
			courageLevel = Convert.ToSingle(generator.NextDouble());
			stupidityLevel = Convert.ToSingle(generator.NextDouble());
			isVeteran = generator.Next(100) == 0;
			isBadass = generator.Next(10) == 0;
			lastName = CrewGenerator.GetLastName();
			levelString = Localizer.Format("#autoLOC_6002246");
		}

		private void SetAttributes()
		{
		}

		private void Lock()
		{
			nameField.interactable = false;
			randomButton.interactable = false;
			maleGender.interactable = false;
			femaleGender.interactable = false;
			pilotRole.interactable = false;
			scientistRole.interactable = false;
			engineerRole.interactable = false;
			touristRole.interactable = false;
			experienceSlider.interactable = false;
			courageSlider.interactable = false;
			stupiditySlider.interactable = false;
			veteranToggle.interactable = false;
			badassToggle.interactable = false;
			submitButton.interactable = false;
		}

		private void Unlock()
		{
			nameField.interactable = true;
			randomButton.interactable = true;
			maleGender.interactable = true;
			femaleGender.interactable = true;
			pilotRole.interactable = true;
			scientistRole.interactable = true;
			engineerRole.interactable = true;
			touristRole.interactable = true;
			experienceSlider.interactable = true;
			courageSlider.interactable = true;
			stupiditySlider.interactable = true;
			veteranToggle.interactable = true;
			badassToggle.interactable = true;
			submitButton.interactable = true;
		}

		private void RefreshControls()
		{
			nameField.text = currentName.Replace(lastName, string.Empty);
			maleGender.isOn = currentGender == ProtoCrewMember.Gender.Male;
			femaleGender.isOn = currentGender == ProtoCrewMember.Gender.Female;
			pilotRole.isOn = currentRole == KerbalRoster.pilotTrait;
			scientistRole.isOn = currentRole == KerbalRoster.scientistTrait;
			engineerRole.isOn = currentRole == KerbalRoster.engineerTrait;
			touristRole.isOn = currentRole == KerbalRoster.touristTrait;
			experienceSlider.minValue = 0f;
			experienceSlider.maxValue = 5f;
			experienceSlider.wholeNumbers = true;
			experienceSlider.value = experienceLevel;
			experienceSliderText.text = levelString + " " + experienceLevel;
			courageSlider.minValue = 0f;
			courageSlider.maxValue = 1f;
			courageSlider.value = courageLevel;
			stupiditySlider.minValue = 0f;
			stupiditySlider.maxValue = 1f;
			stupiditySlider.value = stupidityLevel;
			veteranToggle.isOn = isVeteran;
			badassToggle.isOn = isBadass;
		}

		private void AddListeners()
		{
			nameField.onEndEdit.AddListener(OnNameEdit);
			randomButton.onClick.AddListener(OnRandomClicked);
			maleGender.onValueChanged.AddListener(OnGenderToggle);
			femaleGender.onValueChanged.AddListener(OnGenderToggle);
			pilotRole.onValueChanged.AddListener(OnClassToggle);
			scientistRole.onValueChanged.AddListener(OnClassToggle);
			engineerRole.onValueChanged.AddListener(OnClassToggle);
			touristRole.onValueChanged.AddListener(OnClassToggle);
			experienceSlider.onValueChanged.AddListener(OnExperienceLevelSet);
			courageSlider.onValueChanged.AddListener(OnCourageLevelSet);
			stupiditySlider.onValueChanged.AddListener(OnStupidityLevelSet);
			veteranToggle.onValueChanged.AddListener(OnVeteranToggle);
			badassToggle.onValueChanged.AddListener(OnBadassToggle);
			submitButton.onClick.AddListener(OnSubmitClicked);
		}

		private void KerbalsModified(ProtoCrewMember pcm)
		{
			CheckForErrors();
		}

		private void KerbalsModified(ProtoCrewMember pcm, int count)
		{
			CheckForErrors();
		}

		private void GameLoaded(ConfigNode config)
		{
			CheckForErrors();
		}

		private void CheckForErrors()
		{
			if (!HighLogic.CurrentGame.Parameters.CustomParams<GameParameters.AdvancedParams>().KerbalExperienceEnabled(HighLogic.CurrentGame.Mode))
			{
				experienceLevel = 5;
				experienceSlider.value = experienceLevel;
				experienceSliderText.text = levelString + experienceLevel;
				experienceSlider.interactable = false;
			}
			bool flag = HighLogic.CurrentGame.CrewRoster.Exists(currentName);
			int activeCrewCount = HighLogic.CurrentGame.CrewRoster.GetActiveCrewCount();
			int num = int.MaxValue;
			if (HighLogic.CurrentGame.Mode != 0)
			{
				num = GameVariables.Instance.GetActiveCrewLimit(ScenarioUpgradeableFacilities.GetFacilityLevel(SpaceCenterFacility.AstronautComplex));
			}
			if (!flag && activeCrewCount < num)
			{
				submitButton.interactable = true;
				submitButtonText.text = Localizer.Format("#autoLOC_900441");
				return;
			}
			submitButton.interactable = false;
			if (flag)
			{
				submitButtonText.text = Localizer.Format("#autoLOC_901066");
			}
			else
			{
				submitButtonText.text = Localizer.Format("#autoLOC_901065");
			}
		}

		private void OnNameEdit(string value)
		{
			currentName = CrewGenerator.GetFullName(value, lastName);
			CheckForErrors();
		}

		private void OnRandomClicked()
		{
			currentName = CrewGenerator.GetRandomName(currentGender, generator);
			RefreshControls();
			CheckForErrors();
		}

		private void OnGenderToggle(bool on)
		{
			if (femaleGender.isOn)
			{
				currentGender = ProtoCrewMember.Gender.Female;
			}
			else
			{
				currentGender = ProtoCrewMember.Gender.Male;
			}
		}

		private void OnClassToggle(bool on)
		{
			if (scientistRole.isOn)
			{
				currentRole = KerbalRoster.scientistTrait;
			}
			else if (engineerRole.isOn)
			{
				currentRole = KerbalRoster.engineerTrait;
			}
			else if (touristRole.isOn)
			{
				currentRole = KerbalRoster.touristTrait;
			}
			else
			{
				currentRole = KerbalRoster.pilotTrait;
			}
		}

		private void OnExperienceLevelSet(float value)
		{
			experienceLevel = Mathf.RoundToInt(value);
			experienceSliderText.text = levelString + " " + experienceLevel;
		}

		private void OnCourageLevelSet(float value)
		{
			courageLevel = value;
		}

		private void OnStupidityLevelSet(float value)
		{
			stupidityLevel = value;
		}

		private void OnVeteranToggle(bool on)
		{
			isVeteran = on;
		}

		private void OnBadassToggle(bool on)
		{
			isBadass = on;
		}

		private void OnSubmitClicked()
		{
			ProtoCrewMember protoCrewMember = new ProtoCrewMember(ProtoCrewMember.KerbalType.Crew, currentName);
			protoCrewMember.rosterStatus = ProtoCrewMember.RosterStatus.Available;
			protoCrewMember.gender = currentGender;
			protoCrewMember.trait = currentRole;
			protoCrewMember.courage = courageLevel;
			protoCrewMember.stupidity = stupidityLevel;
			protoCrewMember.veteran = isVeteran;
			protoCrewMember.isBadass = isBadass;
			protoCrewMember.hasToured = false;
			KerbalRoster.SetExperienceLevel(protoCrewMember, experienceLevel);
			if (!HighLogic.CurrentGame.CrewRoster.AddCrewMember(protoCrewMember))
			{
				UnityEngine.Debug.LogError("Cannot create kerbal \"" + base.name + "\".");
			}
			HighLogic.fetch.StartCoroutine(SubmitConfirmation());
		}

		private IEnumerator SubmitConfirmation()
		{
			Lock();
			submitButtonText.text = Localizer.Format("#autoLOC_901067");
			yield return new WaitForSecondsRealtime(2f);
			submitButtonText.text = Localizer.Format("#autoLOC_900441");
			Unlock();
			InitializeLocals();
			RefreshControls();
			CheckForErrors();
		}
	}
}

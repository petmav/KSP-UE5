using TMPro;
using UnityEngine.UI;

namespace Expansions.Missions.Editor
{
	[MEGUI_Asteroid]
	public class MEGUIParameterAsteroid : MEGUICompoundParameter
	{
		public TMP_InputField textSeed;

		public Button buttonRegenSeed;

		private MEGUIParameterDropdownList dropDownAsteroidClass;

		private MEGUIParameterDropdownList dropDownAsteroidType;

		private MEGUIParameterInputField inputName;

		private Asteroid asteroid;

		protected GAPPrefabDisplay prefabDisplay;

		public Asteroid FieldValue
		{
			get
			{
				return asteroid;
			}
			set
			{
				asteroid = value;
				field.SetValue(value);
			}
		}

		protected override void Start()
		{
			base.Start();
			if (FieldValue.persistentId == 0)
			{
				FieldValue.Randomize();
			}
			textSeed.text = FieldValue.seed.ToString();
			inputName.inputField.text = FieldValue.name;
			textSeed.onValueChanged.AddListener(InputText_NewSeed);
		}

		protected override void Setup(string name, object value)
		{
			base.Setup(name, value);
			asteroid = value as Asteroid;
			dropDownAsteroidClass = subParameters["asteroidClass"] as MEGUIParameterDropdownList;
			dropDownAsteroidType = subParameters["asteroidType"] as MEGUIParameterDropdownList;
			inputName = subParameters["name"] as MEGUIParameterInputField;
			buttonRegenSeed.onClick.AddListener(Button_RegenerateSeed);
			dropDownAsteroidClass.dropdownList.onValueChanged.AddListener(RefreshGapPrefab);
			dropDownAsteroidType.dropdownList.onValueChanged.AddListener(RefreshGapPrefab);
			textSeed.contentType = TMP_InputField.ContentType.IntegerNumber;
			textSeed.characterLimit = 9;
		}

		internal void Button_RegenerateSeed()
		{
			uint asteroidSeed = Asteroid.GetAsteroidSeed();
			FieldValue.seed = asteroidSeed;
			textSeed.text = asteroidSeed.ToString();
			RefreshGapPrefab();
		}

		public override void DisplayGAP()
		{
			base.DisplayGAP();
			prefabDisplay = MissionEditorLogic.Instance.actionPane.GAPInitialize<GAPPrefabDisplay>();
			RefreshGapPrefab();
		}

		private void InputText_NewSeed(string inputValue)
		{
			FieldValue.seed = uint.Parse(inputValue);
			RefreshGapPrefab();
		}

		private void RefreshGapPrefab(int foo)
		{
			RefreshGapPrefab();
		}

		private void RefreshGapPrefab()
		{
			prefabDisplay.Setup(MissionsUtils.MEPrefab("Prefabs/GAP_ProceduralAsteroid.prefab"), 10f);
			prefabDisplay.PrefabInstance.GetComponent<GAPProceduralAsteroid>().Setup((int)FieldValue.seed, FieldValue.asteroidType == Asteroid.AsteroidType.Glimmeroid, FieldValue.asteroidClass);
		}
	}
}

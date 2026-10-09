using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar.Screens.Physics
{
	public class ScreenKerbalEVAMaterial : MonoBehaviour
	{
		public TextMeshProUGUI headerText;

		public Button applyChanges;

		private float dynamicFriction;

		public float dynamicFrictionMin;

		public float dynamicFrictionMax;

		public Slider dynamicFrictionSlider;

		public TextMeshProUGUI dynamicFrictionText;

		public TextMeshProUGUI dynamicFrictionCurrentValueText;

		private float staticFriction;

		public float staticFrictionMin;

		public float staticFrictionMax;

		public Slider staticFrictionSlider;

		public TextMeshProUGUI staticFrictionText;

		public TextMeshProUGUI staticFrictionCurrentValueText;

		private float bounciness;

		public float bouncinessMin;

		public float bouncinessMax;

		public Slider bouncinessSlider;

		public TextMeshProUGUI bouncinessText;

		public TextMeshProUGUI bouncinessCurrentValueText;

		private PhysicMaterialCombine frictionCombine;

		public TextMeshProUGUI frictionCombineText;

		public TextMeshProUGUI currentFrictionCombineText;

		public Button frictionAverageButton;

		public Button frictionMinimumButton;

		public Button frictionMultiplyButton;

		public Button frictionMaximumButton;

		private PhysicMaterialCombine bounceCombine;

		public TextMeshProUGUI bounceCombineText;

		public TextMeshProUGUI currentBounceCombineText;

		public Button bounceAverageButton;

		public Button bounceMinimumButton;

		public Button bounceMultiplyButton;

		public Button bounceMaximumButton;

		private void Start()
		{
			dynamicFrictionSlider.minValue = dynamicFrictionMin;
			dynamicFrictionSlider.maxValue = dynamicFrictionMax;
			staticFrictionSlider.minValue = staticFrictionMin;
			staticFrictionSlider.maxValue = staticFrictionMax;
			bouncinessSlider.minValue = bouncinessMin;
			bouncinessSlider.maxValue = bouncinessMax;
			UpdateValuesFromPhysiscsGlobals();
			SlidersUpdated(0f);
			AddListeners();
		}

		private void AddListeners()
		{
			dynamicFrictionSlider.onValueChanged.AddListener(SlidersUpdated);
			staticFrictionSlider.onValueChanged.AddListener(SlidersUpdated);
			bouncinessSlider.onValueChanged.AddListener(SlidersUpdated);
			frictionAverageButton.onClick.AddListener(delegate
			{
				UpdateFrictionCombine(PhysicMaterialCombine.Average, currentFrictionCombineText);
			});
			frictionMinimumButton.onClick.AddListener(delegate
			{
				UpdateFrictionCombine(PhysicMaterialCombine.Minimum, currentFrictionCombineText);
			});
			frictionMultiplyButton.onClick.AddListener(delegate
			{
				UpdateFrictionCombine(PhysicMaterialCombine.Multiply, currentFrictionCombineText);
			});
			frictionMaximumButton.onClick.AddListener(delegate
			{
				UpdateFrictionCombine(PhysicMaterialCombine.Maximum, currentFrictionCombineText);
			});
			bounceAverageButton.onClick.AddListener(delegate
			{
				UpdateBounceCombine(PhysicMaterialCombine.Average, currentBounceCombineText);
			});
			bounceMinimumButton.onClick.AddListener(delegate
			{
				UpdateBounceCombine(PhysicMaterialCombine.Minimum, currentBounceCombineText);
			});
			bounceMultiplyButton.onClick.AddListener(delegate
			{
				UpdateBounceCombine(PhysicMaterialCombine.Multiply, currentBounceCombineText);
			});
			bounceMaximumButton.onClick.AddListener(delegate
			{
				UpdateBounceCombine(PhysicMaterialCombine.Maximum, currentBounceCombineText);
			});
			applyChanges.onClick.AddListener(ApplyChanges);
		}

		private void SlidersUpdated(float arg)
		{
			dynamicFriction = dynamicFrictionSlider.value;
			staticFriction = staticFrictionSlider.value;
			bounciness = bouncinessSlider.value;
			dynamicFrictionCurrentValueText.text = dynamicFriction.ToString("F2");
			staticFrictionCurrentValueText.text = staticFriction.ToString("F2");
			bouncinessCurrentValueText.text = bounciness.ToString("F2");
		}

		private void UpdateValuesFromPhysiscsGlobals()
		{
			dynamicFriction = PhysicsGlobals.KerbalEVADynamicFriction;
			dynamicFrictionSlider.value = dynamicFriction;
			staticFriction = PhysicsGlobals.KerbalEVAStaticFriction;
			staticFrictionSlider.value = staticFriction;
			bounciness = PhysicsGlobals.KerbalEVABounciness;
			bouncinessSlider.value = bounciness;
			dynamicFrictionCurrentValueText.text = dynamicFriction.ToString("F2");
			staticFrictionCurrentValueText.text = staticFriction.ToString("F2");
			bouncinessCurrentValueText.text = bounciness.ToString("F2");
			UpdateFrictionCombine(PhysicsGlobals.KerbalEVAFrictionCombine, currentFrictionCombineText);
			UpdateBounceCombine(PhysicsGlobals.KerbalEVABounceCombine, currentBounceCombineText);
		}

		private void UpdateFrictionCombine(PhysicMaterialCombine newValue, TextMeshProUGUI currentText)
		{
			frictionCombine = newValue;
			currentText.text = "Current: " + frictionCombine;
		}

		private void UpdateBounceCombine(PhysicMaterialCombine newValue, TextMeshProUGUI currentText)
		{
			bounceCombine = newValue;
			currentText.text = "Current: " + bounceCombine;
		}

		private void ApplyChanges()
		{
			if (PhysicsGlobals.Instance != null)
			{
				PhysicsGlobals.Instance.UpdateKerbalEVAPhysicsMaterial(dynamicFriction, staticFriction, bounciness, frictionCombine, bounceCombine);
			}
		}
	}
}

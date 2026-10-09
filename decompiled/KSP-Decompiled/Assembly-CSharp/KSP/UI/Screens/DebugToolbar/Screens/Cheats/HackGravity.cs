using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar.Screens.Cheats
{
	public class HackGravity : MonoBehaviour
	{
		public Toggle toggle;

		public Slider factorSlider;

		public TextMeshProUGUI text;

		public Button resetButton;

		public double gravityFactor = 1.0;

		public double minGravity = 0.01;

		public double maxGravity = 10.0;

		protected bool hacked;

		protected Dictionary<CelestialBody, double> originalGeeForces = new Dictionary<CelestialBody, double>();

		protected bool dirty;

		protected virtual void Awake()
		{
			SetupValues();
			hacked = false;
			toggle.isOn = false;
			toggle.onValueChanged.AddListener(OnToggleChanged);
			factorSlider.onValueChanged.AddListener(OnSliderChanged);
			factorSlider.interactable = false;
			factorSlider.value = 1f;
			resetButton.onClick.AddListener(OnResetClick);
			resetButton.interactable = false;
		}

		protected virtual void Update()
		{
			if (originalGeeForces.Count != FlightGlobals.Bodies.Count)
			{
				CreateOriginalGeeForces();
				if (hacked)
				{
					SetGravityFactor(gravityFactor);
				}
			}
		}

		protected virtual void OnToggleChanged(bool state)
		{
			factorSlider.interactable = state;
			resetButton.interactable = state;
			if (state)
			{
				SetGravityFactor(factorSlider.value);
				SetHack(hack: true);
			}
			else
			{
				SetHack(hack: false);
			}
		}

		protected virtual void OnSliderChanged(float value)
		{
			SetGravityFactor(value);
			SetText();
		}

		public virtual void OnResetClick()
		{
			SetGravityFactor(1.0);
			SetupValues();
		}

		protected virtual void SetupValues()
		{
			factorSlider.minValue = (float)minGravity;
			factorSlider.maxValue = (float)maxGravity;
			factorSlider.value = (float)gravityFactor;
			SetText();
		}

		protected virtual void SetText()
		{
			text.text = KSPUtil.LocalizeNumber(gravityFactor, "F2");
		}

		protected virtual void CreateOriginalGeeForces()
		{
			originalGeeForces.Clear();
			int i = 0;
			for (int count = FlightGlobals.Bodies.Count; i < count; i++)
			{
				CelestialBody celestialBody = FlightGlobals.Bodies[i];
				originalGeeForces[celestialBody] = celestialBody.GeeASL;
			}
		}

		protected virtual void SetHack(bool hack)
		{
			if (hack)
			{
				if (!hacked)
				{
					SetGravityFactor(gravityFactor);
					hacked = true;
				}
			}
			else if (hacked)
			{
				SetGravityFactor(1.0);
				hacked = false;
			}
		}

		protected virtual void SetGravityFactor(double factor)
		{
			int i = 0;
			for (int count = FlightGlobals.Bodies.Count; i < count; i++)
			{
				CelestialBody celestialBody = FlightGlobals.Bodies[i];
				celestialBody.GeeASL = originalGeeForces[celestialBody] * factor;
			}
			gravityFactor = factor;
		}
	}
}

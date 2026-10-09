using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar
{
	public class DebugScreenCheats : MonoBehaviour
	{
		public Toggle hackGravity;

		public Slider hackGravityFactor;

		public TextMeshProUGUI hackGravityText;

		public Toggle pauseOnVesselUnpack;

		public Toggle unbreakableJoints;

		public Toggle noCrashDamage;

		public Toggle ignoreMaxTemperature;

		public Toggle infinitePropellant;

		public Toggle infiniteElectricity;

		public Toggle biomesVisibleInMap;

		public Toggle allowPartClippingInEditors;

		public Toggle nonStrictPartAttachmentOrientationChecks;

		public static DebugScreenCheats Instance { get; private set; }

		private void Awake()
		{
			Instance = this;
		}

		private void OnDestroy()
		{
			Instance = null;
		}
	}
}

using TMPro;
using UnityEngine;

namespace KSP.UI.Screens.DebugToolbar.Screens.Debug
{
	public class ScreenDeltaVPropellantInfo : MonoBehaviour
	{
		[SerializeField]
		private TextMeshProUGUI propellantName;

		[SerializeField]
		private TextMeshProUGUI amountNumber;

		[SerializeField]
		private TextMeshProUGUI engineBurnsNumber;

		public DeltaVPropellantInfo propellantInfo;

		public void UpdateData(DeltaVPropellantInfo propellantInfo, CalcType type)
		{
			this.propellantInfo = propellantInfo;
			propellantName.text = propellantInfo.propellant.displayName;
			amountNumber.text = propellantInfo.amountAvailable.ToString("N2");
			if ((uint)type > 1u && type == CalcType.Actual)
			{
				if (propellantInfo.amountPerSecondCurrentThrottle <= 0.0)
				{
					engineBurnsNumber.text = propellantInfo.amountPerSecondSetThrottle.ToString("N4");
				}
				else
				{
					engineBurnsNumber.text = propellantInfo.amountPerSecondCurrentThrottle.ToString("N4");
				}
			}
			else
			{
				engineBurnsNumber.text = propellantInfo.amountPerSecondSetThrottle.ToString("N4");
			}
		}
	}
}

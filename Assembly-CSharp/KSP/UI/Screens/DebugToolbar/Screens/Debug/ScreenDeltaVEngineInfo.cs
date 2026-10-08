using KSP.Localization;
using TMPro;
using UnityEngine;

namespace KSP.UI.Screens.DebugToolbar.Screens.Debug
{
	public class ScreenDeltaVEngineInfo : MonoBehaviour
	{
		[SerializeField]
		private TextMeshProUGUI engineName;

		[SerializeField]
		private TextMeshProUGUI engineType;

		[SerializeField]
		private TextMeshProUGUI maxThrustNumber;

		[SerializeField]
		private TextMeshProUGUI maxThrustType;

		[SerializeField]
		private TextMeshProUGUI ispNumber;

		[SerializeField]
		private TextMeshProUGUI ispType;

		[SerializeField]
		private TextMeshProUGUI maxBurnTimeNumber;

		public DeltaVEngineInfo engineInfo;

		public void UpdateData(DeltaVEngineInfo engineInfo, CalcType type, string typeDesc, int stage)
		{
			this.engineInfo = engineInfo;
			engineName.text = engineInfo.engine.part.partInfo.title;
			engineType.text = engineInfo.engine.GetEngineType().ToString();
			switch (type)
			{
			default:
				maxThrustNumber.text = Localizer.Format("#autoLOC_8002207", engineInfo.thrustVac.ToString("N2"));
				maxThrustType.text = type.displayDescription();
				ispNumber.text = engineInfo.ispVac.ToString("N2") + Localizer.Format("#autoLOC_6002317");
				ispType.text = typeDesc;
				maxBurnTimeNumber.text = KSPUtil.dateTimeFormatter.PrintTimeCompact(engineInfo.GetFuelTimeAtThrottle(stage), explicitPositive: false);
				break;
			case CalcType.ASL:
				maxThrustNumber.text = Localizer.Format("#autoLOC_8002207", engineInfo.thrustASL.ToString("N2"));
				maxThrustType.text = type.displayDescription();
				ispNumber.text = engineInfo.ispASL.ToString("N2") + Localizer.Format("#autoLOC_6002317");
				ispType.text = typeDesc;
				maxBurnTimeNumber.text = KSPUtil.dateTimeFormatter.PrintTimeCompact(engineInfo.GetFuelTimeAtThrottle(stage), explicitPositive: false);
				break;
			case CalcType.Actual:
				maxThrustNumber.text = Localizer.Format("#autoLOC_8002207", engineInfo.thrustActual.ToString("N2"));
				maxThrustType.text = type.displayDescription();
				ispNumber.text = engineInfo.ispActual.ToString("N2") + Localizer.Format("#autoLOC_6002317");
				ispType.text = typeDesc;
				maxBurnTimeNumber.text = KSPUtil.dateTimeFormatter.PrintTimeCompact(engineInfo.GetFuelTimeAtActiveThrottle(stage), explicitPositive: false);
				break;
			}
		}
	}
}

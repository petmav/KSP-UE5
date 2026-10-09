using UnityEngine;

namespace KSP.UI.Screens.Flight
{
	public class SASDisplay : MonoBehaviour
	{
		public UIStateImage stateImage;

		public UIStateText stateText;

		private void Reset()
		{
			stateImage = GetComponent<UIStateImage>();
			stateText = GetComponent<UIStateText>();
		}

		private void Start()
		{
			LateUpdate();
		}

		private void SetNewState(string newState)
		{
			stateImage.SetState(newState);
			stateText.SetState(newState);
		}

		private void LateUpdate()
		{
			if (!FlightGlobals.ready)
			{
				return;
			}
			Vessel activeVessel = FlightGlobals.ActiveVessel;
			if (activeVessel.isEVA)
			{
				if (GameSettings.EVA_ROTATE_ON_MOVE)
				{
					SetNewState("On");
				}
				else
				{
					SetNewState("Off");
				}
			}
			else if (activeVessel.ActionGroups[KSPActionGroup.SAS])
			{
				if (activeVessel.Autopilot.SAS.dampingMode)
				{
					SetNewState("Override");
				}
				else
				{
					SetNewState("On");
				}
			}
			else
			{
				SetNewState("Off");
			}
		}
	}
}

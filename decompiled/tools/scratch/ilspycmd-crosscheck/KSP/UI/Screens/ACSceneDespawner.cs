using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public class ACSceneDespawner : MonoBehaviour
	{
		public Button exitButton;

		private void Start()
		{
			exitButton.onClick.AddListener(BtnExit);
		}

		private void BtnExit()
		{
			GameEvents.onGUIAstronautComplexDespawn.Fire();
		}
	}
}

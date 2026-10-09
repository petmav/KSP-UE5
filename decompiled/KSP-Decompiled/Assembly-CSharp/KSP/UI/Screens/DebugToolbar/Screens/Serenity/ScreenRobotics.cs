using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar.Screens.Serenity
{
	public class ScreenRobotics : MonoBehaviour
	{
		public Toggle dataActionMenus;

		private void Start()
		{
			dataActionMenus.isOn = PhysicsGlobals.RoboticJointDataDisplay;
			AddListeners();
		}

		private void AddListeners()
		{
			dataActionMenus.onValueChanged.AddListener(OnDataActionMenusToggle);
		}

		private void OnDataActionMenusToggle(bool on)
		{
			PhysicsGlobals.RoboticJointDataDisplay = on;
		}
	}
}

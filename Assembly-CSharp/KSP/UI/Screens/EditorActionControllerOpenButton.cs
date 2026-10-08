using Expansions.Serenity;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public class EditorActionControllerOpenButton : UISelectableGridLayoutGroupItem
	{
		public Button openButton;

		private ModuleRoboticController controller;

		public void Setup(ModuleRoboticController controller)
		{
			this.controller = controller;
			openButton.onClick.AddListener(OpenButtonClicked);
		}

		private void OpenButtonClicked()
		{
			if (RoboticControllerManager.Instance != null)
			{
				RoboticControllerWindow.Spawn(controller);
			}
		}
	}
}

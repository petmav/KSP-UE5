using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	internal class EditorSaveFolderItem : MonoBehaviour
	{
		public Toggle toggleSetDefault;

		public string saveFolderPath;

		public TextMeshProUGUI textName;

		public Button buttonSave;

		private UICraftSaveFlyoutController controller;

		public bool IsOpenFileBrowserRequest;

		private void Awake()
		{
			buttonSave.onClick.AddListener(MouseInput_OnClick);
			toggleSetDefault.onValueChanged.AddListener(MouseInput_SelectDefault);
		}

		private void OnDestroy()
		{
			buttonSave.onClick.RemoveAllListeners();
			toggleSetDefault.onValueChanged.RemoveAllListeners();
		}

		public void MouseInput_OnClick()
		{
			if (IsOpenFileBrowserRequest)
			{
				controller.OpenCraftFolderBrowser();
			}
			else
			{
				SaveCraft();
			}
			controller.CloseFlyout();
		}

		public void MouseInput_SelectDefault(bool value)
		{
			if (!IsOpenFileBrowserRequest && value && !EditorDriver.SetDefaultSaveFolder(saveFolderPath))
			{
				ScreenMessages.PostScreenMessage(Localizer.Format("#autoLOC_6009024"), 5f);
			}
		}

		public void Create(UICraftSaveFlyoutController controller, string saveFolderPath, string displayName)
		{
			this.controller = controller;
			this.saveFolderPath = saveFolderPath;
			SetPathDisplay(displayName);
		}

		public bool IsPath(string path)
		{
			return saveFolderPath == path;
		}

		public void SetPathDisplay(string displayName)
		{
			textName.text = displayName.Replace("\\", "/");
		}

		public void SetPath(string fullPath)
		{
			saveFolderPath = fullPath;
		}

		public void SaveCraft()
		{
			ShipConstruction.SaveShipToPath(EditorLogic.fetch.shipNameField.text, saveFolderPath);
			controller.UpdateFolderDisplay(saveFolderPath);
		}
	}
}

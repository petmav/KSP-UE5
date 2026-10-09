using System.IO;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI
{
	public class DialogUISelectFile : MonoBehaviour
	{
		public delegate void OnFileSelected(FileInfo file);

		public DialogUISelectFileItem itemPrefab;

		public TextMeshProUGUI textTitle;

		public TextMeshProUGUI textDescription;

		public RectTransform layout;

		public Button buttonAccept;

		public Button buttonCancel;

		public string filePath = "";

		public string fileSearchPattern = "*.sfs";

		public OnFileSelected onFileSelected;

		private FileInfo selectedItem;

		private FileInfo[] files;

		private void Awake()
		{
			buttonAccept.onClick.AddListener(OnAccept);
			buttonCancel.onClick.AddListener(OnCancel);
		}

		private void Start()
		{
			base.transform.SetParent(UIMasterController.Instance.dialogCanvas.transform, worldPositionStays: false);
			UpdateFiles();
		}

		public void Setup(string title, string description, string filePath, string fileSearchPattern, OnFileSelected onFileSelected)
		{
			textTitle.text = title;
			textDescription.text = description;
			this.filePath = filePath;
			this.fileSearchPattern = fileSearchPattern;
			this.onFileSelected = onFileSelected;
		}

		public void UpdateFiles()
		{
			int childCount = layout.childCount;
			while (childCount-- > 0)
			{
				Object.Destroy(layout.GetChild(childCount).gameObject);
			}
			files = null;
			if (!Directory.Exists(filePath))
			{
				Debug.LogError("DialogGUISelectFile: Directory does not exist");
				return;
			}
			DirectoryInfo directoryInfo = new DirectoryInfo(filePath);
			files = directoryInfo.GetFiles(fileSearchPattern, SearchOption.TopDirectoryOnly);
			int i = 0;
			for (int num = files.Length; i < num; i++)
			{
				FileInfo fileInfo = files[i];
				int itemIndex = i;
				DialogUISelectFileItem dialogUISelectFileItem = Object.Instantiate(itemPrefab);
				dialogUISelectFileItem.transform.SetParent(layout, worldPositionStays: false);
				dialogUISelectFileItem.textTitle.text = Path.GetFileNameWithoutExtension(fileInfo.Name);
				dialogUISelectFileItem.radioButton.SetState(UIRadioButton.State.False, UIRadioButton.CallType.APPLICATIONSILENT, null, popButtonsInGroup: false);
				dialogUISelectFileItem.radioButton.SetGroup(90876234);
				dialogUISelectFileItem.radioButton.onTrue.AddListener(delegate
				{
					SelectItem(itemIndex);
				});
			}
		}

		public void Destroy()
		{
			Object.Destroy(base.gameObject);
		}

		private void OnAccept()
		{
			onFileSelected(selectedItem);
			Destroy();
		}

		private void OnCancel()
		{
			onFileSelected(null);
			Destroy();
		}

		private void SelectItem(int index)
		{
			selectedItem = files[index];
		}
	}
}

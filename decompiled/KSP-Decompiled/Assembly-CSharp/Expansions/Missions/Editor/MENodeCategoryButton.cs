using KSP.UI.TooltipTypes;
using RUI.Icons.Selectable;
using UnityEngine;
using UnityEngine.UI;

namespace Expansions.Missions.Editor
{
	public class MENodeCategoryButton : MonoBehaviour
	{
		public RawImage buttonImage;

		public Button categoryButton;

		public string categoryName;

		public string categoryDisplayName;

		public TooltipController_Text tooltipController;

		public Sprite normalState;

		public Sprite selectedState;

		[SerializeField]
		private Image highlighter;

		private MENodeCategorizer nodeCategorizer;

		private Icon icon;

		private Color colorWhite = new Color(1f, 1f, 1f, 0.7f);

		private Color colorBlack = new Color(0f, 0f, 0f, 0.7f);

		public void Setup(string newCategoryName, string newCategoryDisplayName, MENodeCategorizer newNodeCategorizer, Icon icon)
		{
			this.icon = icon;
			categoryName = newCategoryName;
			categoryDisplayName = newCategoryDisplayName;
			tooltipController.textString = newCategoryDisplayName;
			nodeCategorizer = newNodeCategorizer;
			categoryButton.onClick.AddListener(OnMouseClick);
			buttonImage.texture = icon.iconNormal;
			DeselectCategory();
		}

		private void OnMouseClick()
		{
			nodeCategorizer.DisplayNodesInCategory(categoryName);
		}

		public void DeselectCategory()
		{
			SetIcon(selected: false);
		}

		public void SelectCategory()
		{
			SetIcon(selected: true);
		}

		private void SetIcon(bool selected)
		{
			if (icon.simple)
			{
				if (selected)
				{
					buttonImage.texture = icon.iconSelected;
					buttonImage.color = colorWhite;
					categoryButton.image.sprite = selectedState;
				}
				else
				{
					buttonImage.texture = icon.iconSelected;
					buttonImage.color = colorBlack;
					categoryButton.image.sprite = normalState;
				}
			}
			else if (selected)
			{
				buttonImage.texture = icon.iconSelected;
				buttonImage.color = colorWhite;
				categoryButton.image.sprite = selectedState;
			}
			else
			{
				buttonImage.texture = icon.iconNormal;
				buttonImage.color = colorWhite;
				categoryButton.image.sprite = normalState;
			}
		}

		public void ToggleHighlighter(bool state)
		{
			if (highlighter != null)
			{
				highlighter.gameObject.SetActive(state);
			}
		}
	}
}

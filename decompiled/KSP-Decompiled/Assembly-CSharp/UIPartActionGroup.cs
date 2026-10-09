using System.Collections.Generic;
using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

[UI_Group]
public class UIPartActionGroup : UIPartActionItem
{
	[SerializeField]
	private Button button;

	[SerializeField]
	private TextMeshProUGUI groupHeader;

	[SerializeField]
	private RectTransform content;

	[SerializeField]
	private CanvasGroup contentCanvasGroup;

	[SerializeField]
	private LayoutElement contentLayout;

	[SerializeField]
	private Image collapseStateImage;

	[SerializeField]
	private Sprite collapseSprite;

	[SerializeField]
	private Sprite expandSprite;

	private List<Transform> contentItems = new List<Transform>();

	private string groupName;

	private string groupDisplayName;

	private bool isContentCollapsed;

	private bool initialized;

	private void Start()
	{
		button.onClick.AddListener(CollapseGroupToggle);
		GameEvents.onUIScaleChange.Add(RescaleContentItems);
	}

	private void OnDestroy()
	{
		button.onClick.RemoveListener(CollapseGroupToggle);
		GameEvents.onUIScaleChange.Remove(RescaleContentItems);
	}

	public void Initialize(string groupName, string groupDisplayName, bool startCollapsed, UIPartActionWindow pawWindow)
	{
		this.groupName = groupName;
		this.groupDisplayName = Localizer.Format(groupDisplayName);
		window = pawWindow;
		isContentCollapsed = startCollapsed || GameSettings.collpasedPAWGroups.Contains(groupName);
		groupHeader.text = this.groupDisplayName;
		SetUIState();
		initialized = true;
	}

	private void CollapseGroupToggle()
	{
		if (isContentCollapsed)
		{
			Expand();
		}
		else
		{
			Collapse();
		}
	}

	private void SetUIState()
	{
		contentLayout.ignoreLayout = isContentCollapsed;
		contentCanvasGroup.alpha = ((!isContentCollapsed) ? 1 : 0);
		contentCanvasGroup.interactable = !isContentCollapsed;
		contentCanvasGroup.blocksRaycasts = !isContentCollapsed;
		collapseStateImage.sprite = (isContentCollapsed ? expandSprite : collapseSprite);
	}

	public void Collapse()
	{
		if (initialized)
		{
			isContentCollapsed = true;
			GameSettings.collpasedPAWGroups.AddUnique(groupName);
			GameSettings.SaveSettingsOnNextGameSave();
			SetUIState();
			Canvas.ForceUpdateCanvases();
			window.UpdateWindow();
		}
	}

	public void Expand()
	{
		if (initialized)
		{
			isContentCollapsed = false;
			GameSettings.collpasedPAWGroups.Remove(groupName);
			GameSettings.SaveSettingsOnNextGameSave();
			SetUIState();
			Canvas.ForceUpdateCanvases();
			window.UpdateWindow();
		}
	}

	public void AddItemToContent(Transform t)
	{
		contentItems.Add(t);
		t.SetParent(content.transform);
		UpdateContentSize();
	}

	private void RescaleContentItems()
	{
		for (int i = 0; i < contentItems.Count; i++)
		{
			if (contentItems[i] != null)
			{
				contentItems[i].localScale = Vector3.one;
				RectTransform rectTransform = contentItems[i] as RectTransform;
				if (rectTransform != null)
				{
					rectTransform.anchoredPosition3D = Vector3.zero;
				}
			}
			else
			{
				contentItems.RemoveAt(i);
			}
		}
	}

	public void UpdateContentSize()
	{
		for (int i = 0; i < contentItems.Count; i++)
		{
			if (contentItems[i] == null)
			{
				contentItems.RemoveAt(i);
			}
		}
		if (contentItems.Count <= 0)
		{
			base.Window.RemoveGroup(groupName);
			Object.Destroy(base.gameObject);
		}
		else
		{
			RescaleContentItems();
			Canvas.ForceUpdateCanvases();
			SetUIState();
		}
	}
}

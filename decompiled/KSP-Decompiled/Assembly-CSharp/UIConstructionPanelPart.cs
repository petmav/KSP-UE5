using UnityEngine;
using UnityEngine.UI;

public class UIConstructionPanelPart : MonoBehaviour
{
	public RectTransform containerChilden;

	public Button buttonCollapse;

	public Sprite spriteCollapseOn;

	public Sprite spriteCollapseOff;

	private void Start()
	{
		buttonCollapse.onClick.AddListener(OnButtonCollapsePressed);
	}

	private void OnDestroy()
	{
		buttonCollapse.onClick.RemoveListener(OnButtonCollapsePressed);
	}

	private void Update()
	{
	}

	private void OnButtonCollapsePressed()
	{
		containerChilden.gameObject.SetActive(!containerChilden.gameObject.activeSelf);
		((Image)buttonCollapse.targetGraphic).sprite = (containerChilden.gameObject.activeSelf ? spriteCollapseOff : spriteCollapseOn);
	}
}

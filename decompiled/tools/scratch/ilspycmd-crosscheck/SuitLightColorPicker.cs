using UnityEngine;
using UnityEngine.UI;

public class SuitLightColorPicker : MonoBehaviour
{
	public ColorPicker colorPicker;

	public Canvas colorControls;

	[SerializeField]
	private Button colorPickerButton;

	[SerializeField]
	private GameObject blocker;

	public ProtoCrewMember crew;

	[SerializeField]
	private SuitButton suitButton;

	private void Start()
	{
		colorPickerButton.onClick.AddListener(OnPickColorButton);
		colorPicker.onValueChanged.AddListener(OnColorChange);
	}

	private void OnDestroy()
	{
		colorPickerButton.onClick.RemoveListener(OnPickColorButton);
		colorPicker.onValueChanged.RemoveListener(OnColorChange);
	}

	private void OnPickColorButton()
	{
		colorControls.enabled = !colorControls.enabled;
		if (colorControls.enabled)
		{
			blocker = new GameObject("blocker", typeof(Button), typeof(Image));
			blocker.GetComponent<Button>().onClick.AddListener(OnPickColorButton);
			blocker.GetComponent<Image>().color = Color.clear;
			RectTransform obj = blocker.transform as RectTransform;
			obj.SetParent(GetComponentInParent<Canvas>().transform, worldPositionStays: false);
			obj.anchorMin = Vector2.zero;
			obj.anchorMax = Vector2.one;
			obj.offsetMin = Vector2.zero;
			obj.offsetMax = Vector2.zero;
			obj.anchoredPosition = Vector3.zero;
		}
		else
		{
			Object.Destroy(blocker);
		}
	}

	private void OnColorChange(Color color)
	{
		crew.lightR = color.r;
		crew.lightG = color.g;
		crew.lightB = color.b;
		suitButton.comboSelector.previewBodyMaterial.SetColor("_EmissiveColor", color);
		suitButton.comboSelector.previewHelmetMaterial.SetColor("_EmissiveColor", color);
		suitButton.OnButtonClicked();
	}
}

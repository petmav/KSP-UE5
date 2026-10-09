using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class ComboButton : MonoBehaviour, IPointerEnterHandler, IEventSystemHandler, IPointerExitHandler
{
	[SerializeField]
	private Image imagePrimaryColor;

	[SerializeField]
	private Image imageSecondaryColor;

	[SerializeField]
	private Image imageSelected;

	[SerializeField]
	private Button buttonMain;

	public ComboSelector comboSelector;

	public int comboIndex;

	public string comboId;

	public Image ImagePrimaryColor => imagePrimaryColor;

	public Image ImageSecondaryColor => imageSecondaryColor;

	private void Start()
	{
		buttonMain.onClick.AddListener(OnSuitComboSelection);
	}

	private void OnDestroy()
	{
		buttonMain.onClick.RemoveListener(OnSuitComboSelection);
	}

	public void OnSuitComboSelection()
	{
		GameEvents.onSuitComboSelection.Fire(comboSelector, comboId, comboIndex);
	}

	public void Select(int comboIndex)
	{
		comboSelector.fieldValue = comboIndex;
		imageSelected.gameObject.SetActive(value: true);
	}

	public void Reset()
	{
		imageSelected.gameObject.SetActive(value: false);
	}

	void IPointerEnterHandler.OnPointerEnter(PointerEventData eventData)
	{
	}

	void IPointerExitHandler.OnPointerExit(PointerEventData eventData)
	{
	}
}

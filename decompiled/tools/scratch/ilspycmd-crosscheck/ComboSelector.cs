using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class ComboSelector : MonoBehaviour
{
	[SerializeField]
	private Button buttonPrevious;

	[SerializeField]
	private Button buttonNext;

	[SerializeField]
	private ScrollRect scrollMain;

	public int comboEntryIndex;

	public List<ComboButton> comboList;

	public int fieldValue;

	public Material previewBodyMaterial;

	public Material previewHelmetMaterial;

	public Material previewNeckringMaterial;

	public SuitButton suitButton;

	public ScrollRect ScrollMain => scrollMain;

	private void Awake()
	{
		comboList = new List<ComboButton>();
		buttonPrevious.onClick.AddListener(OnButtonPrev);
		buttonNext.onClick.AddListener(OnButtonNext);
	}

	private void OnDestroy()
	{
		buttonPrevious.onClick.RemoveListener(OnButtonPrev);
		buttonNext.onClick.RemoveListener(OnButtonNext);
	}

	private void OnButtonPrev()
	{
		int newIndex = (fieldValue + comboList.Count - 1) % comboList.Count;
		SelectVariant(newIndex);
	}

	private void OnButtonNext()
	{
		int newIndex = (fieldValue + 1) % comboList.Count;
		SelectVariant(newIndex);
	}

	private void SelectVariant(int newIndex)
	{
		GameEvents.onSuitComboSelection.Fire(this, comboList[newIndex].comboId, newIndex);
		fieldValue = newIndex;
	}
}

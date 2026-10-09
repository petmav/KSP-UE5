using TMPro;
using UnityEngine;
using UnityEngine.UI;

[UI_Label]
public class UIPartActionResourcePriority : UIPartActionItem
{
	[SerializeField]
	private TextMeshProUGUI txtPriority;

	[SerializeField]
	private TextMeshProUGUI txtPriorityOffset;

	[SerializeField]
	private Button btnDec;

	[SerializeField]
	private Button btnInc;

	[SerializeField]
	private Button btnReset;

	private int resPriority = int.MaxValue;

	private int resPriorityOffset = int.MaxValue;

	public virtual void Setup(UIPartActionWindow window, Part part, UI_Scene scene)
	{
		SetupItem(window, part, null, scene, null);
		btnDec.onClick.AddListener(OnDecClick);
		btnInc.onClick.AddListener(OnIncClick);
		btnReset.onClick.AddListener(OnResetClick);
	}

	private void Awake()
	{
	}

	public override void UpdateItem()
	{
		int resourcePriority = part.GetResourcePriority();
		if (resPriority != resourcePriority)
		{
			resPriority = resourcePriority;
			txtPriority.text = resPriority.ToString();
		}
		int resourcePriorityOffset = part.resourcePriorityOffset;
		if (resPriorityOffset != resourcePriorityOffset)
		{
			resPriorityOffset = resourcePriorityOffset;
			txtPriorityOffset.text = "(";
			if (resourcePriorityOffset > 0)
			{
				txtPriorityOffset.text += "+";
			}
			TextMeshProUGUI textMeshProUGUI = txtPriorityOffset;
			textMeshProUGUI.text = textMeshProUGUI.text + resPriorityOffset + ")";
		}
	}

	private void OnDecClick()
	{
		if (GameSettings.MODIFIER_KEY.GetKey())
		{
			part.ChangeResourcePriority(-10);
		}
		else
		{
			part.ChangeResourcePriority(-1);
		}
	}

	private void OnIncClick()
	{
		if (GameSettings.MODIFIER_KEY.GetKey())
		{
			part.ChangeResourcePriority(10);
		}
		else
		{
			part.ChangeResourcePriority(1);
		}
	}

	private void OnResetClick()
	{
		part.ResetPri();
	}
}

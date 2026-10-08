using UnityEngine;

public class GraphicsProfiles : MonoBehaviour
{
	public Transform profileDropDown;

	public UIButton dropper;

	public SpriteText currentSel;

	public UIButton minButton;

	public UIButton lowButton;

	public UIButton normButton;

	public UIButton highButton;

	public UIButton maxButton;

	public GraphicsScreen gs;

	private void Start()
	{
		profileDropDown.gameObject.SetActive(value: false);
		currentSel.Text = "Custom";
		dropper.AddValueChangedDelegate(profileChangedDel);
		minButton.AddValueChangedDelegate(profileChangedDel);
		lowButton.AddValueChangedDelegate(profileChangedDel);
		normButton.AddValueChangedDelegate(profileChangedDel);
		highButton.AddValueChangedDelegate(profileChangedDel);
		maxButton.AddValueChangedDelegate(profileChangedDel);
	}

	private void profileChangedDel(IUIObject obj)
	{
		if (obj == dropper)
		{
			profileDropDown.gameObject.SetActive(!profileDropDown.gameObject.activeInHierarchy);
		}
		else if (obj == minButton)
		{
			gs.setGraphicsProfile("MINIMUM");
			profileDropDown.gameObject.SetActive(value: false);
			currentSel.Text = "Minimum";
		}
		else if (obj == lowButton)
		{
			gs.setGraphicsProfile("LOW");
			profileDropDown.gameObject.SetActive(value: false);
			currentSel.Text = "Low";
		}
		else if (obj == normButton)
		{
			gs.setGraphicsProfile("NORMAL");
			profileDropDown.gameObject.SetActive(value: false);
			currentSel.Text = "Normal";
		}
		else if (obj == highButton)
		{
			gs.setGraphicsProfile("HIGH");
			profileDropDown.gameObject.SetActive(value: false);
			currentSel.Text = "High";
		}
		else if (obj == maxButton)
		{
			gs.setGraphicsProfile("MAXIMUM");
			profileDropDown.gameObject.SetActive(value: false);
			currentSel.Text = "Maximum";
		}
	}
}

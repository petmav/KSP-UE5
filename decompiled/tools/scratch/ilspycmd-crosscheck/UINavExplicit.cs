using UnityEngine;
using UnityEngine.UI;

public class UINavExplicit : MonoBehaviour
{
	private Selectable selectable;

	private UINavMouseChecker uiNavMouse;

	private int lastItemSelected;

	private void Start()
	{
		selectable = GetComponent<Selectable>();
		uiNavMouse = GetComponent<UINavMouseChecker>();
		GameEvents.onMenuNavGetInput.Add(CheckForNonInteractable);
	}

	private void OnDestroy()
	{
		GameEvents.onMenuNavGetInput.Remove(CheckForNonInteractable);
	}

	private void CheckForNonInteractable(MenuNavInput input)
	{
		if (uiNavMouse.index == uiNavMouse.menuNav.lastItemSelected)
		{
			if (!selectable.IsInteractable())
			{
				uiNavMouse.menuNav.lastItemSelected = lastItemSelected;
				uiNavMouse.menuNav.SelectLastArrowSelected();
			}
		}
		else
		{
			lastItemSelected = uiNavMouse.menuNav.lastItemSelected;
		}
	}
}

using UnityEngine;

public class ColliderMenuMouseChecker : MonoBehaviour
{
	public MainMenu mainMenu;

	private bool mouseOver;

	public void OnMouseEnter()
	{
		mouseOver = true;
		MouseIsHovering();
	}

	public void OnMouseExit()
	{
		mouseOver = false;
		MouseHasExited();
	}

	private void MouseIsHovering()
	{
		if ((bool)mainMenu)
		{
			mainMenu.MouseIsHovering(mouseOver);
		}
	}

	private void MouseHasExited()
	{
		if ((bool)mainMenu)
		{
			mainMenu.MouseIsHovering(mouseOver);
		}
	}
}

using UnityEngine;

public class CreditsSceneMain : MonoBehaviour
{
	private void returnToMainMenu()
	{
		HighLogic.LoadScene(GameScenes.MAINMENU);
	}

	private void Update()
	{
		if (Input.GetKeyDown(KeyCode.Escape))
		{
			returnToMainMenu();
		}
	}
}

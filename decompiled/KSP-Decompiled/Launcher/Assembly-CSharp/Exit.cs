using UnityEngine;

public class Exit : MonoBehaviour
{
	public Texture2D cursorTexture;

	private void Awake()
	{
		if (GameObject.FindGameObjectsWithTag("Exit").Length > 1)
		{
			Object.Destroy(base.gameObject);
		}
		Object.DontDestroyOnLoad(base.gameObject);
	}

	private void OnMouseEnter()
	{
		Cursor.SetCursor(cursorTexture, new Vector2(5f, 0f), CursorMode.Auto);
	}

	private void OnMouseExit()
	{
		Cursor.SetCursor(null, Vector2.zero, CursorMode.Auto);
	}

	private void OnMouseDown()
	{
		QuitLauncher();
	}

	private void QuitLauncher()
	{
		Application.Quit();
	}
}

using UnityEngine;

public class Settings : MonoBehaviour
{
	public Texture2D cursorTexture;

	public Texture2D onTex;

	public Texture2D offTex;

	public bool allowGoToSettings;

	private void Start()
	{
		allowGoToSettings = true;
	}

	private void Update()
	{
		setTexture();
	}

	private void OnMouseEnter()
	{
		if (allowGoToSettings && Application.platform != RuntimePlatform.LinuxPlayer)
		{
			Cursor.SetCursor(cursorTexture, new Vector2(5f, 0f), CursorMode.Auto);
		}
	}

	private void OnMouseExit()
	{
		if (allowGoToSettings && Application.platform != RuntimePlatform.LinuxPlayer)
		{
			Cursor.SetCursor(null, Vector2.zero, CursorMode.Auto);
		}
	}

	private void OnMouseDown()
	{
		if (allowGoToSettings)
		{
			Application.LoadLevel(1);
		}
	}

	private void setTexture()
	{
		if (allowGoToSettings)
		{
			GetComponent<Renderer>().material.mainTexture = onTex;
		}
		else
		{
			GetComponent<Renderer>().material.mainTexture = offTex;
		}
	}
}

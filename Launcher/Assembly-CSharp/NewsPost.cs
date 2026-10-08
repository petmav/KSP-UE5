using UnityEngine;

public class NewsPost : MonoBehaviour
{
	private string date;

	private string link;

	private string title;

	private string body;

	public Texture2D cursorTexture;

	public void DrawNewsPost()
	{
		foreach (Transform item in base.transform)
		{
			switch (item.gameObject.name)
			{
			case "Date":
				item.GetComponent<TextMesh>().text = date + " -";
				break;
			case "Title":
				item.GetComponent<TextMesh>().text = title;
				break;
			case "Body":
				item.GetComponent<TextMesh>().text = body;
				break;
			}
		}
	}

	private void OnMouseEnter()
	{
		if (Application.platform != RuntimePlatform.LinuxPlayer)
		{
			Cursor.SetCursor(cursorTexture, new Vector2(5f, 0f), CursorMode.Auto);
		}
	}

	private void OnMouseExit()
	{
		if (Application.platform != RuntimePlatform.LinuxPlayer)
		{
			Cursor.SetCursor(null, Vector2.zero, CursorMode.Auto);
		}
	}

	private void OnMouseDown()
	{
		Application.OpenURL(link);
	}

	public void setDate(string date)
	{
		this.date = date;
	}

	public void setLink(string link)
	{
		this.link = link;
	}

	public void setTitle(string title)
	{
		this.title = title;
	}

	public void setBody(string body)
	{
		this.body = body;
	}
}

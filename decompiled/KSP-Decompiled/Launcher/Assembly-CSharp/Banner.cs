using System.Collections;
using System.IO;
using UnityEngine;

public class Banner : MonoBehaviour
{
	private string bannerImageURL = "https://kerbalspaceprogram.com/launcher/banner.png";

	public string siteURL = "https://www.kerbalspaceprogram.com/game/kerbal-space-program/";

	public string buyKSPURL = "https://www.kerbalspaceprogram.com/store/";

	public Texture2D cursorTexture;

	private void Awake()
	{
		StartCoroutine(GetImage());
	}

	private IEnumerator GetImage()
	{
		WWW connection = new WWW(bannerImageURL);
		yield return connection;
		if (connection.error != null)
		{
			MonoBehaviour.print(connection.error);
		}
		else
		{
			GetComponent<Renderer>().material.mainTexture = connection.texture;
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
		bool flag = false;
		if (File.Exists((Application.platform == RuntimePlatform.OSXPlayer) ? Path.Combine(Application.dataPath, "../../buildID.txt") : Path.Combine(Application.dataPath, "../buildID.txt")))
		{
			string[] array = File.ReadAllLines((Application.platform == RuntimePlatform.OSXPlayer) ? Path.Combine(Application.dataPath, "../../buildID.txt") : Path.Combine(Application.dataPath, "../buildID.txt"));
			for (int i = 0; i < array.Length; i++)
			{
				if (array[i].Contains("Branch: NewDemo"))
				{
					flag = true;
				}
			}
		}
		if (flag)
		{
			Application.OpenURL(buyKSPURL);
		}
		else
		{
			Application.OpenURL(siteURL);
		}
	}
}

using System.Collections;
using UnityEngine;

public class Background : MonoBehaviour
{
	private string backgroundImageURL = "https://kerbalspaceprogram.com/launcher/background.jpg";

	private void Awake()
	{
		if (GameObject.FindGameObjectsWithTag("Logic").Length > 1)
		{
			Object.Destroy(base.gameObject);
		}
		Object.DontDestroyOnLoad(this);
		StartCoroutine(GetImage());
	}

	private IEnumerator GetImage()
	{
		WWW connection = new WWW(backgroundImageURL);
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
}

using System.Collections;
using System.Collections.Generic;
using System.Net;
using System.Text.RegularExpressions;
using System.Xml;
using UnityEngine;
using UnityEngine.Networking;

public class News : MonoBehaviour
{
	public string newsURL;

	private XmlDocument news;

	private List<string[]> posts;

	private void Awake()
	{
		news = new XmlDocument();
		StartCoroutine("GetNews");
	}

	private IEnumerator GetNews()
	{
		ServicePointManager.SecurityProtocol = SecurityProtocolType.Ssl3;
		UnityWebRequest www = new UnityWebRequest(newsURL)
		{
			downloadHandler = new DownloadHandlerBuffer()
		};
		yield return www.SendWebRequest();
		if (www.isNetworkError || www.isHttpError)
		{
			Debug.Log(www.error);
			yield break;
		}
		news.LoadXml(www.downloadHandler.text);
		posts = new List<string[]>();
		XmlNodeList elementsByTagName = news.GetElementsByTagName("post");
		for (int i = 0; i < elementsByTagName.Count; i++)
		{
			string[] item = new string[4]
			{
				cleanDate(elementsByTagName[i].Attributes["date"].InnerText),
				elementsByTagName[i].Attributes["url"].InnerText,
				cleanTitle(elementsByTagName[i]["regular-title"].InnerText),
				cleanBody(elementsByTagName[i]["regular-body"].InnerText)
			};
			posts.Add(item);
		}
		DrawNews();
	}

	private void DrawNews()
	{
		for (int i = 0; i < posts.Count; i++)
		{
			NewsPost component = GameObject.Find("NewsPost" + (i + 1)).GetComponent<NewsPost>();
			component.setDate(posts[i][0]);
			component.setLink(posts[i][1]);
			component.setTitle(posts[i][2]);
			component.setBody(posts[i][3]);
			component.DrawNewsPost();
		}
	}

	public string cleanDate(string date)
	{
		int startIndex = date.IndexOf(" ");
		date = date.Substring(startIndex, 12);
		string[] array = date.Split(' ');
		date = array[2] + " " + array[1] + ", " + array[3];
		return date;
	}

	public string cleanBody(string body)
	{
		body = Regex.Replace(body, "<.*?>", string.Empty);
		if (body.IndexOf("\n") == 0)
		{
			body = body.Substring(1);
		}
		body = body.Replace("\n", " ");
		body = body.Replace("&#8217;", "'");
		if (body.Length > 65)
		{
			body = body.Substring(0, 65);
		}
		if (body.Length != 0)
		{
			int length = body.LastIndexOf(" ");
			body = body.Substring(0, length) + "...";
		}
		return body;
	}

	public string cleanTitle(string body)
	{
		body = Regex.Replace(body, "<.*?>", string.Empty);
		if (body.IndexOf("\n") == 0)
		{
			body = body.Substring(1);
		}
		body = body.Replace("\n", " ");
		body = body.Replace("&#8217;", "'");
		if (body.Length > 60)
		{
			body = body.Substring(0, 60);
		}
		if (body.Length != 0)
		{
			int num = body.LastIndexOf(" ");
			if (num > 0)
			{
				body = body.Substring(0, num) + "...";
			}
		}
		return body;
	}
}

using UnityEngine;

public class KSPSite : MonoBehaviour
{
	public KSPSiteType type;

	public Texture2D cursorTexture;

	public StatsCallHome stcallhome;

	private string siteURL;

	private Color[] colors;

	private void Awake()
	{
		SetURLs();
		SetColors();
	}

	private void SetURLs()
	{
		switch (type)
		{
		case KSPSiteType.KSPSTORE:
			siteURL = "https://kerbalspaceprogram.com/kspstore";
			break;
		case KSPSiteType.SPACEPORT:
			siteURL = "https://www.curseforge.com/kerbal/ksp-mods";
			break;
		case KSPSiteType.FACEBOOK:
			siteURL = "https://www.facebook.com/pages/Kerbal-Space-Program/240760572609841";
			break;
		case KSPSiteType.TWITTER:
			siteURL = "https://twitter.com/KerbalSpaceP";
			break;
		case KSPSiteType.YOUTUBE:
			siteURL = "http://www.youtube.com/user/KerbalSPOfficial";
			break;
		case KSPSiteType.REDDIT:
			siteURL = "http://www.reddit.com/r/kerbalspaceprogram";
			break;
		case KSPSiteType.TWITCH:
			siteURL = "http://twitch.tv/ksptv";
			break;
		case KSPSiteType.FORUMS:
			siteURL = "https://forum.kerbalspaceprogram.com/";
			break;
		}
	}

	private void SetColors()
	{
		colors = new Color[8];
		colors[0] = new Color(1f, 1f, 1f);
		colors[1] = new Color(1f, 1f, 1f);
		colors[2] = new Color(0.019f, 0.298f, 0.847f);
		colors[3] = new Color(0.122f, 0.58f, 0.906f);
		colors[4] = new Color(0.737f, 0.09f, 0.133f);
		colors[5] = new Color(0.976f, 0.353f, 0.027f);
		colors[6] = new Color(0.576f, 0.074f, 0.792f);
		colors[7] = new Color(0.737f, 0.847f, 0.141f);
	}

	private void OnMouseEnter()
	{
		if (Application.platform != RuntimePlatform.LinuxPlayer)
		{
			Cursor.SetCursor(cursorTexture, new Vector2(5f, 0f), CursorMode.Auto);
		}
		switch (type)
		{
		case KSPSiteType.KSPSTORE:
			GetComponent<Renderer>().material.SetColor("_Color", colors[0]);
			break;
		case KSPSiteType.SPACEPORT:
			GetComponent<Renderer>().material.SetColor("_Color", colors[1]);
			break;
		case KSPSiteType.FACEBOOK:
			GetComponent<Renderer>().material.SetColor("_Color", colors[2]);
			break;
		case KSPSiteType.TWITTER:
			GetComponent<Renderer>().material.SetColor("_Color", colors[3]);
			break;
		case KSPSiteType.YOUTUBE:
			GetComponent<Renderer>().material.SetColor("_Color", colors[4]);
			break;
		case KSPSiteType.REDDIT:
			GetComponent<Renderer>().material.SetColor("_Color", colors[5]);
			break;
		case KSPSiteType.TWITCH:
			GetComponent<Renderer>().material.SetColor("_Color", colors[6]);
			break;
		case KSPSiteType.FORUMS:
			GetComponent<Renderer>().material.SetColor("_Color", colors[7]);
			break;
		}
	}

	private void OnMouseExit()
	{
		if (Application.platform != RuntimePlatform.LinuxPlayer)
		{
			Cursor.SetCursor(null, Vector2.zero, CursorMode.Auto);
		}
		GetComponent<Renderer>().material.SetColor("_Color", new Color(1f, 1f, 1f));
	}

	private void OnMouseDown()
	{
		stcallhome.SendInfo(type.ToString());
		Application.OpenURL(siteURL);
	}
}

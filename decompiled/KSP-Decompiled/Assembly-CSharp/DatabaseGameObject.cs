using System.Collections.Generic;
using KSP.FX.Fireworks;
using UnityEngine;

public class DatabaseGameObject : MonoBehaviour
{
	private static List<string> databases = new List<string>();

	public string databaseName = string.Empty;

	[SerializeField]
	public static FireworkFXList fwFXList;

	private void Awake()
	{
		if (databaseName != string.Empty)
		{
			if (databases.Contains(databaseName))
			{
				Object.DestroyImmediate(base.gameObject);
				return;
			}
			databases.Add(databaseName);
		}
		Object.DontDestroyOnLoad(base.gameObject);
		fwFXList = new FireworkFXList();
		LoadFireworkFXDefinitions();
	}

	public void LoadFireworkFXDefinitions()
	{
		fwFXList.fireworkFX.Clear();
		ConfigNode[] array = GameDatabase.Instance?.GetConfigNodes("FIREWORKFX_DEFINITION");
		int num = ((array != null) ? array.Length : 0);
		for (int i = 0; i < num; i++)
		{
			fwFXList.Add(array[i]);
		}
	}
}

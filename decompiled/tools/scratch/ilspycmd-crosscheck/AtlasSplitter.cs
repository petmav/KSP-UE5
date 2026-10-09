using System;
using UnityEngine;

public class AtlasSplitter : MonoBehaviour
{
	[Serializable]
	public class Atlas
	{
		public Texture2D[] tex;

		public int xCount;

		public int yCount;
	}

	public Atlas[] atlases;

	public string outputPath;

	private void Reset()
	{
		outputPath = "_UI/Editor/";
	}
}

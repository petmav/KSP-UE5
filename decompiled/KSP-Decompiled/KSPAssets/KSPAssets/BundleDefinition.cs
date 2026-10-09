using System;
using System.Collections.Generic;
using System.IO;
using System.Xml.Serialization;
using UnityEngine;

namespace KSPAssets
{
	[Serializable]
	[XmlRoot("KSPBundleDefinition")]
	public class BundleDefinition
	{
		[XmlAttribute("UrlName")]
		public string urlName = "";

		[XmlAttribute("Name")]
		public string name = "";

		[XmlIgnore]
		public string path = "";

		[XmlAttribute("CreatedTime")]
		public long createdTime = -1L;

		[XmlAttribute("AutoLoad")]
		public bool autoLoad = false;

		[XmlElement("Author")]
		public string author = "";

		[XmlElement("Info")]
		public string info = "";

		[XmlArray("Assets")]
		[XmlArrayItem("Asset")]
		public List<AssetDefinition> assets = new List<AssetDefinition>();

		[XmlArray("Dependencies")]
		[XmlArrayItem("Dependency")]
		public List<string> dependencyNames = new List<string>();

		[NonSerialized]
		[XmlIgnore]
		public List<BundleDefinition> dependencyBundles = new List<BundleDefinition>();

		[XmlIgnore]
		public bool dependenciesFound = false;

		public void Save(string path)
		{
			string directoryName = Path.GetDirectoryName(path);
			if (!Directory.Exists(directoryName))
			{
				Directory.CreateDirectory(directoryName);
			}
			using (StreamWriter streamWriter = new StreamWriter(path))
			{
				XmlSerializer xmlSerializer = new XmlSerializer(GetType());
				xmlSerializer.Serialize(streamWriter, this);
				streamWriter.Flush();
			}
		}

		public static BundleDefinition LoadFromFile(string path)
		{
			try
			{
				using (FileStream stream = File.OpenRead(path))
				{
					XmlSerializer xmlSerializer = new XmlSerializer(typeof(BundleDefinition));
					return xmlSerializer.Deserialize(stream) as BundleDefinition;
				}
			}
			catch
			{
				Debug.LogError("KSPBundleDefinition: Could not deserialise from '" + path + "'");
				return null;
			}
		}

		public static BundleDefinition CreateFromText(string text)
		{
			try
			{
				using (MemoryStream stream = GenerateStreamFromString(text))
				{
					XmlSerializer xmlSerializer = new XmlSerializer(typeof(BundleDefinition));
					return xmlSerializer.Deserialize(stream) as BundleDefinition;
				}
			}
			catch
			{
				Debug.LogError("KSPBundleDefinition: Could not deserialise from text");
				return null;
			}
		}

		private static MemoryStream GenerateStreamFromString(string s)
		{
			MemoryStream memoryStream = new MemoryStream();
			StreamWriter streamWriter = new StreamWriter(memoryStream);
			streamWriter.Write(s);
			streamWriter.Flush();
			memoryStream.Position = 0L;
			return memoryStream;
		}

		public AssetDefinition GetAssetWithUrl(string assetUrl)
		{
			for (int i = 0; i < assets.Count; i++)
			{
				if (assets[i].url == assetUrl)
				{
					return assets[i];
				}
			}
			return null;
		}

		public AssetDefinition GetAssetWithPath(string assetPath)
		{
			string text = assetPath.ToLower();
			for (int i = 0; i < assets.Count; i++)
			{
				if (assets[i].path == text || assets[i].path == assetPath)
				{
					return assets[i];
				}
			}
			return null;
		}

		public AssetDefinition GetAssetWithName(string assetName)
		{
			for (int i = 0; i < assets.Count; i++)
			{
				if (assets[i].name == assetName)
				{
					return assets[i];
				}
			}
			return null;
		}

		public AssetDefinition[] GetAssetsWithName(string assetName)
		{
			List<AssetDefinition> list = new List<AssetDefinition>();
			for (int i = 0; i < assets.Count; i++)
			{
				if (assets[i].name == assetName)
				{
					list.Add(assets[i]);
				}
			}
			return list.ToArray();
		}

		public AssetDefinition GetAssetWithBundleName(string bundleName)
		{
			for (int i = 0; i < assets.Count; i++)
			{
				if (assets[i].bundle.name == bundleName)
				{
					return assets[i];
				}
			}
			return null;
		}

		public AssetDefinition[] GetAssetsWithBundleName(string bundleName)
		{
			List<AssetDefinition> list = new List<AssetDefinition>();
			for (int i = 0; i < assets.Count; i++)
			{
				if (assets[i].bundle.name == bundleName)
				{
					list.Add(assets[i]);
				}
			}
			return list.ToArray();
		}

		public AssetDefinition GetAssetWithType(string assetType)
		{
			for (int i = 0; i < assets.Count; i++)
			{
				if (assets[i].type == assetType)
				{
					return assets[i];
				}
			}
			return null;
		}

		public AssetDefinition GetAssetWithType(Type assetType)
		{
			return GetAssetWithType(assetType.Name);
		}

		public AssetDefinition[] GetAssetsWithType(string assetType)
		{
			List<AssetDefinition> list = new List<AssetDefinition>();
			for (int i = 0; i < assets.Count; i++)
			{
				if (assets[i].type == assetType)
				{
					list.Add(assets[i]);
				}
			}
			return list.ToArray();
		}

		public AssetDefinition[] GetAssetsWithType(Type assetType)
		{
			return GetAssetsWithType(assetType.Name);
		}

		public AssetDefinition GetAssetWithNameOfType(string assetName, string assetType)
		{
			for (int i = 0; i < assets.Count; i++)
			{
				if (assets[i].name == assetName && assets[i].type == assetType)
				{
					return assets[i];
				}
			}
			return null;
		}

		public AssetDefinition GetAssetWithNameOfType(string assetName, Type type)
		{
			return GetAssetWithNameOfType(assetName, type.Name);
		}
	}
}

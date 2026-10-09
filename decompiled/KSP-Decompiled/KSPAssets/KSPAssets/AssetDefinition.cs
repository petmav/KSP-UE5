using System;
using System.Xml.Serialization;

namespace KSPAssets
{
	[Serializable]
	public class AssetDefinition
	{
		[XmlAttribute("Name")]
		public string name = "";

		[XmlAttribute("Path")]
		public string path = "";

		[XmlAttribute("Type")]
		public string type = "";

		[XmlAttribute("Url")]
		public string url = "";

		[XmlAttribute("AutoLoad")]
		public bool autoLoad = false;

		[NonSerialized]
		[XmlIgnore]
		public BundleDefinition bundle = null;

		public AssetDefinition()
		{
		}

		public AssetDefinition(string path, string name, string type)
		{
			this.path = path;
			this.name = name;
			this.type = type;
			url = path.Substring(0, path.IndexOf('.')).Substring(7);
		}

		public override string ToString()
		{
			return "(" + type + ") " + name + " : '" + url + "'";
		}
	}
}

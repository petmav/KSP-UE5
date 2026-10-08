using System;
using System.Collections;
using System.Xml.Serialization;
using KSPAssets;
using KSPAssets.KSPedia;

namespace Microsoft.Xml.Serialization.GeneratedAssembly
{
	public class XmlSerializerContract : XmlSerializerImplementation
	{
		private Hashtable readMethods;

		private Hashtable writeMethods;

		private Hashtable typedSerializers;

		public override XmlSerializationReader Reader => new XmlSerializationReader1();

		public override XmlSerializationWriter Writer => new XmlSerializationWriter1();

		public override Hashtable ReadMethods
		{
			get
			{
				if (readMethods == null)
				{
					Hashtable hashtable = new Hashtable();
					hashtable["KSPAssets.AssetDefinition::"] = "Read8_AssetDefinition";
					hashtable["KSPAssets.BundleDefinition::KSPBundleDefinition:True:"] = "Read9_KSPBundleDefinition";
					hashtable["KSPAssets.KSPedia.KSPediaDatabase::KSPedia:True:"] = "Read10_KSPedia";
					hashtable["KSPAssets.KSPedia.KSPediaDatabase+Screen::"] = "Read11_Screen";
					hashtable["KSPAssets.KSPedia.KSPediaDatabase+ScreenText::"] = "Read12_ScreenText";
					hashtable["KSPAssets.KSPedia.KSPediaDatabase+Category::"] = "Read13_Category";
					if (readMethods == null)
					{
						readMethods = hashtable;
					}
				}
				return readMethods;
			}
		}

		public override Hashtable WriteMethods
		{
			get
			{
				if (writeMethods == null)
				{
					Hashtable hashtable = new Hashtable();
					hashtable["KSPAssets.AssetDefinition::"] = "Write8_AssetDefinition";
					hashtable["KSPAssets.BundleDefinition::KSPBundleDefinition:True:"] = "Write9_KSPBundleDefinition";
					hashtable["KSPAssets.KSPedia.KSPediaDatabase::KSPedia:True:"] = "Write10_KSPedia";
					hashtable["KSPAssets.KSPedia.KSPediaDatabase+Screen::"] = "Write11_Screen";
					hashtable["KSPAssets.KSPedia.KSPediaDatabase+ScreenText::"] = "Write12_ScreenText";
					hashtable["KSPAssets.KSPedia.KSPediaDatabase+Category::"] = "Write13_Category";
					if (writeMethods == null)
					{
						writeMethods = hashtable;
					}
				}
				return writeMethods;
			}
		}

		public override Hashtable TypedSerializers
		{
			get
			{
				if (typedSerializers == null)
				{
					Hashtable hashtable = new Hashtable();
					hashtable.Add("KSPAssets.AssetDefinition::", new AssetDefinitionSerializer());
					hashtable.Add("KSPAssets.KSPedia.KSPediaDatabase::KSPedia:True:", new KSPediaDatabaseSerializer());
					hashtable.Add("KSPAssets.KSPedia.KSPediaDatabase+ScreenText::", new ScreenTextSerializer());
					hashtable.Add("KSPAssets.KSPedia.KSPediaDatabase+Category::", new CategorySerializer());
					hashtable.Add("KSPAssets.KSPedia.KSPediaDatabase+Screen::", new ScreenSerializer());
					hashtable.Add("KSPAssets.BundleDefinition::KSPBundleDefinition:True:", new BundleDefinitionSerializer());
					if (typedSerializers == null)
					{
						typedSerializers = hashtable;
					}
				}
				return typedSerializers;
			}
		}

		public override bool CanSerialize(Type type)
		{
			if (type == typeof(AssetDefinition))
			{
				return true;
			}
			if (type == typeof(BundleDefinition))
			{
				return true;
			}
			if (type == typeof(KSPediaDatabase))
			{
				return true;
			}
			if (type == typeof(KSPediaDatabase.Screen))
			{
				return true;
			}
			if (type == typeof(KSPediaDatabase.ScreenText))
			{
				return true;
			}
			if (type == typeof(KSPediaDatabase.Category))
			{
				return true;
			}
			return false;
		}

		public override XmlSerializer GetSerializer(Type type)
		{
			if (type == typeof(AssetDefinition))
			{
				return new AssetDefinitionSerializer();
			}
			if (type == typeof(BundleDefinition))
			{
				return new BundleDefinitionSerializer();
			}
			if (type == typeof(KSPediaDatabase))
			{
				return new KSPediaDatabaseSerializer();
			}
			if (type == typeof(KSPediaDatabase.Screen))
			{
				return new ScreenSerializer();
			}
			if (type == typeof(KSPediaDatabase.ScreenText))
			{
				return new ScreenTextSerializer();
			}
			if (type == typeof(KSPediaDatabase.Category))
			{
				return new CategorySerializer();
			}
			return null;
		}
	}
}

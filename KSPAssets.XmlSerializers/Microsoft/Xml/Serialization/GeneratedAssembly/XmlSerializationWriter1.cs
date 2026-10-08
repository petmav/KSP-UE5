using System;
using System.Collections;
using System.Collections.Generic;
using System.Xml;
using System.Xml.Serialization;
using KSPAssets;
using KSPAssets.KSPedia;

namespace Microsoft.Xml.Serialization.GeneratedAssembly
{
	public class XmlSerializationWriter1 : XmlSerializationWriter
	{
		public void Write8_AssetDefinition(object o)
		{
			WriteStartDocument();
			if (o == null)
			{
				WriteNullTagLiteral("AssetDefinition", "");
				return;
			}
			TopLevelElement();
			Write2_AssetDefinition("AssetDefinition", "", (AssetDefinition)o, isNullable: true, needType: false);
		}

		public void Write9_KSPBundleDefinition(object o)
		{
			WriteStartDocument();
			if (o == null)
			{
				WriteNullTagLiteral("KSPBundleDefinition", "");
				return;
			}
			TopLevelElement();
			Write3_BundleDefinition("KSPBundleDefinition", "", (BundleDefinition)o, isNullable: true, needType: false);
		}

		public void Write10_KSPedia(object o)
		{
			WriteStartDocument();
			if (o == null)
			{
				WriteNullTagLiteral("KSPedia", "");
				return;
			}
			TopLevelElement();
			Write7_KSPediaDatabase("KSPedia", "", (KSPediaDatabase)o, isNullable: true, needType: false);
		}

		public void Write11_Screen(object o)
		{
			WriteStartDocument();
			if (o == null)
			{
				WriteNullTagLiteral("Screen", "");
				return;
			}
			TopLevelElement();
			Write6_Screen("Screen", "", (KSPediaDatabase.Screen)o, isNullable: true, needType: false);
		}

		public void Write12_ScreenText(object o)
		{
			WriteStartDocument();
			if (o == null)
			{
				WriteNullTagLiteral("ScreenText", "");
				return;
			}
			TopLevelElement();
			Write5_ScreenText("ScreenText", "", (KSPediaDatabase.ScreenText)o, isNullable: true, needType: false);
		}

		public void Write13_Category(object o)
		{
			WriteStartDocument();
			if (o == null)
			{
				WriteNullTagLiteral("Category", "");
				return;
			}
			TopLevelElement();
			Write4_Category("Category", "", (KSPediaDatabase.Category)o, isNullable: true, needType: false);
		}

		private void Write4_Category(string n, string ns, KSPediaDatabase.Category o, bool isNullable, bool needType)
		{
			if (o == null)
			{
				if (isNullable)
				{
					WriteNullTagLiteral(n, ns);
				}
				return;
			}
			if (!needType)
			{
				Type type = o.GetType();
				if (!(type == typeof(KSPediaDatabase.Category)))
				{
					throw CreateUnknownTypeException(o);
				}
			}
			WriteStartElement(n, ns, o, writePrefixed: false, null);
			if (needType)
			{
				WriteXsiType("Category", "");
			}
			WriteAttribute("Name", "", o.name);
			WriteElementString("Title", "", o.title);
			WriteElementString("TitleScreen", "", o.titleScreen);
			List<string> screens = o.screens;
			if (screens != null)
			{
				WriteStartElement("Screens", "", null, writePrefixed: false);
				for (int i = 0; i < ((ICollection)screens).Count; i++)
				{
					WriteNullableStringLiteral("Screen", "", screens[i]);
				}
				WriteEndElement();
			}
			List<KSPediaDatabase.Category> subcategories = o.subcategories;
			if (subcategories != null)
			{
				WriteStartElement("Subcategories", "", null, writePrefixed: false);
				for (int j = 0; j < ((ICollection)subcategories).Count; j++)
				{
					Write4_Category("Subcategory", "", subcategories[j], isNullable: true, needType: false);
				}
				WriteEndElement();
			}
			WriteEndElement(o);
		}

		private void Write5_ScreenText(string n, string ns, KSPediaDatabase.ScreenText o, bool isNullable, bool needType)
		{
			if (o == null)
			{
				if (isNullable)
				{
					WriteNullTagLiteral(n, ns);
				}
				return;
			}
			if (!needType)
			{
				Type type = o.GetType();
				if (!(type == typeof(KSPediaDatabase.ScreenText)))
				{
					throw CreateUnknownTypeException(o);
				}
			}
			WriteStartElement(n, ns, o, writePrefixed: false, null);
			if (needType)
			{
				WriteXsiType("ScreenText", "");
			}
			WriteAttribute("Name", "", o.name);
			WriteElementString("Text", "", o.text);
			WriteEndElement(o);
		}

		private void Write6_Screen(string n, string ns, KSPediaDatabase.Screen o, bool isNullable, bool needType)
		{
			if (o == null)
			{
				if (isNullable)
				{
					WriteNullTagLiteral(n, ns);
				}
				return;
			}
			if (!needType)
			{
				Type type = o.GetType();
				if (!(type == typeof(KSPediaDatabase.Screen)))
				{
					throw CreateUnknownTypeException(o);
				}
			}
			WriteStartElement(n, ns, o, writePrefixed: false, null);
			if (needType)
			{
				WriteXsiType("Screen", "");
			}
			WriteAttribute("Name", "", o.name);
			WriteElementString("BundleName", "", o.bundleName);
			WriteElementString("AssetPath", "", o.assetPath);
			WriteElementString("Title", "", o.title);
			WriteElementString("Background", "", o.background);
			List<KSPediaDatabase.ScreenText> text = o.text;
			if (text != null)
			{
				WriteStartElement("Text", "", null, writePrefixed: false);
				for (int i = 0; i < ((ICollection)text).Count; i++)
				{
					Write5_ScreenText("Text", "", text[i], isNullable: true, needType: false);
				}
				WriteEndElement();
			}
			List<KSPediaDatabase.ScreenText> tooltips = o.tooltips;
			if (tooltips != null)
			{
				WriteStartElement("Tooltips", "", null, writePrefixed: false);
				for (int j = 0; j < ((ICollection)tooltips).Count; j++)
				{
					Write5_ScreenText("Tooltip", "", tooltips[j], isNullable: true, needType: false);
				}
				WriteEndElement();
			}
			WriteEndElement(o);
		}

		private void Write7_KSPediaDatabase(string n, string ns, KSPediaDatabase o, bool isNullable, bool needType)
		{
			if (o == null)
			{
				if (isNullable)
				{
					WriteNullTagLiteral(n, ns);
				}
				return;
			}
			if (!needType)
			{
				Type type = o.GetType();
				if (!(type == typeof(KSPediaDatabase)))
				{
					throw CreateUnknownTypeException(o);
				}
			}
			WriteStartElement(n, ns, o, writePrefixed: false, null);
			if (needType)
			{
				WriteXsiType("KSPediaDatabase", "");
			}
			WriteAttribute("Name", "", o.name);
			WriteAttribute("CreatedTime", "", XmlConvert.ToString(o.createdTime));
			List<KSPediaDatabase.Category> categories = o.categories;
			if (categories != null)
			{
				WriteStartElement("Categories", "", null, writePrefixed: false);
				for (int i = 0; i < ((ICollection)categories).Count; i++)
				{
					Write4_Category("Category", "", categories[i], isNullable: true, needType: false);
				}
				WriteEndElement();
			}
			List<KSPediaDatabase.Screen> screens = o.screens;
			if (screens != null)
			{
				WriteStartElement("Screens", "", null, writePrefixed: false);
				for (int j = 0; j < ((ICollection)screens).Count; j++)
				{
					Write6_Screen("Screen", "", screens[j], isNullable: true, needType: false);
				}
				WriteEndElement();
			}
			WriteEndElement(o);
		}

		private void Write3_BundleDefinition(string n, string ns, BundleDefinition o, bool isNullable, bool needType)
		{
			if (o == null)
			{
				if (isNullable)
				{
					WriteNullTagLiteral(n, ns);
				}
				return;
			}
			if (!needType)
			{
				Type type = o.GetType();
				if (!(type == typeof(BundleDefinition)))
				{
					throw CreateUnknownTypeException(o);
				}
			}
			WriteStartElement(n, ns, o, writePrefixed: false, null);
			if (needType)
			{
				WriteXsiType("BundleDefinition", "");
			}
			WriteAttribute("UrlName", "", o.urlName);
			WriteAttribute("Name", "", o.name);
			WriteAttribute("CreatedTime", "", XmlConvert.ToString(o.createdTime));
			WriteElementString("Author", "", o.author);
			WriteElementString("Info", "", o.info);
			List<AssetDefinition> assets = o.assets;
			if (assets != null)
			{
				WriteStartElement("Assets", "", null, writePrefixed: false);
				for (int i = 0; i < ((ICollection)assets).Count; i++)
				{
					Write2_AssetDefinition("Asset", "", assets[i], isNullable: true, needType: false);
				}
				WriteEndElement();
			}
			List<string> dependencyNames = o.dependencyNames;
			if (dependencyNames != null)
			{
				WriteStartElement("Dependencies", "", null, writePrefixed: false);
				for (int j = 0; j < ((ICollection)dependencyNames).Count; j++)
				{
					WriteNullableStringLiteral("Dependency", "", dependencyNames[j]);
				}
				WriteEndElement();
			}
			WriteEndElement(o);
		}

		private void Write2_AssetDefinition(string n, string ns, AssetDefinition o, bool isNullable, bool needType)
		{
			if (o == null)
			{
				if (isNullable)
				{
					WriteNullTagLiteral(n, ns);
				}
				return;
			}
			if (!needType)
			{
				Type type = o.GetType();
				if (!(type == typeof(AssetDefinition)))
				{
					throw CreateUnknownTypeException(o);
				}
			}
			WriteStartElement(n, ns, o, writePrefixed: false, null);
			if (needType)
			{
				WriteXsiType("AssetDefinition", "");
			}
			WriteAttribute("Name", "", o.name);
			WriteAttribute("Path", "", o.path);
			WriteAttribute("Type", "", o.type);
			WriteAttribute("Url", "", o.url);
			WriteAttribute("AutoLoad", "", XmlConvert.ToString(o.autoLoad));
			WriteEndElement(o);
		}

		protected override void InitCallbacks()
		{
		}
	}
}

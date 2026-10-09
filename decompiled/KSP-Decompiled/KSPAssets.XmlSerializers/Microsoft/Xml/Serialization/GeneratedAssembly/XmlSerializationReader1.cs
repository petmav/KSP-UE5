using System.Collections.Generic;
using System.Xml;
using System.Xml.Serialization;
using KSPAssets;
using KSPAssets.KSPedia;

namespace Microsoft.Xml.Serialization.GeneratedAssembly
{
	public class XmlSerializationReader1 : XmlSerializationReader
	{
		private string id21_CreatedTime;

		private string id34_AutoLoad;

		private string id6_ScreenText;

		private string id22_Categories;

		private string id26_Info;

		private string id5_Screen;

		private string id18_Tooltips;

		private string id11_Screens;

		private string id29_Dependencies;

		private string id7_Category;

		private string id3_KSPBundleDefinition;

		private string id23_BundleDefinition;

		private string id30_Dependency;

		private string id31_Path;

		private string id28_Asset;

		private string id1_AssetDefinition;

		private string id2_Item;

		private string id10_TitleScreen;

		private string id13_Subcategory;

		private string id25_Author;

		private string id16_AssetPath;

		private string id8_Name;

		private string id24_UrlName;

		private string id33_Url;

		private string id4_KSPedia;

		private string id19_Tooltip;

		private string id27_Assets;

		private string id15_BundleName;

		private string id17_Background;

		private string id32_Type;

		private string id12_Subcategories;

		private string id20_KSPediaDatabase;

		private string id14_Text;

		private string id9_Title;

		public object Read8_AssetDefinition()
		{
			object result = null;
			base.Reader.MoveToContent();
			if (base.Reader.NodeType == XmlNodeType.Element)
			{
				if ((object)base.Reader.LocalName != id1_AssetDefinition || (object)base.Reader.NamespaceURI != id2_Item)
				{
					throw CreateUnknownNodeException();
				}
				result = Read2_AssetDefinition(isNullable: true, checkType: true);
			}
			else
			{
				UnknownNode(null, ":AssetDefinition");
			}
			return result;
		}

		public object Read9_KSPBundleDefinition()
		{
			object result = null;
			base.Reader.MoveToContent();
			if (base.Reader.NodeType == XmlNodeType.Element)
			{
				if ((object)base.Reader.LocalName != id3_KSPBundleDefinition || (object)base.Reader.NamespaceURI != id2_Item)
				{
					throw CreateUnknownNodeException();
				}
				result = Read3_BundleDefinition(isNullable: true, checkType: true);
			}
			else
			{
				UnknownNode(null, ":KSPBundleDefinition");
			}
			return result;
		}

		public object Read10_KSPedia()
		{
			object result = null;
			base.Reader.MoveToContent();
			if (base.Reader.NodeType == XmlNodeType.Element)
			{
				if ((object)base.Reader.LocalName != id4_KSPedia || (object)base.Reader.NamespaceURI != id2_Item)
				{
					throw CreateUnknownNodeException();
				}
				result = Read7_KSPediaDatabase(isNullable: true, checkType: true);
			}
			else
			{
				UnknownNode(null, ":KSPedia");
			}
			return result;
		}

		public object Read11_Screen()
		{
			object result = null;
			base.Reader.MoveToContent();
			if (base.Reader.NodeType == XmlNodeType.Element)
			{
				if ((object)base.Reader.LocalName != id5_Screen || (object)base.Reader.NamespaceURI != id2_Item)
				{
					throw CreateUnknownNodeException();
				}
				result = Read6_Screen(isNullable: true, checkType: true);
			}
			else
			{
				UnknownNode(null, ":Screen");
			}
			return result;
		}

		public object Read12_ScreenText()
		{
			object result = null;
			base.Reader.MoveToContent();
			if (base.Reader.NodeType == XmlNodeType.Element)
			{
				if ((object)base.Reader.LocalName != id6_ScreenText || (object)base.Reader.NamespaceURI != id2_Item)
				{
					throw CreateUnknownNodeException();
				}
				result = Read5_ScreenText(isNullable: true, checkType: true);
			}
			else
			{
				UnknownNode(null, ":ScreenText");
			}
			return result;
		}

		public object Read13_Category()
		{
			object result = null;
			base.Reader.MoveToContent();
			if (base.Reader.NodeType == XmlNodeType.Element)
			{
				if ((object)base.Reader.LocalName != id7_Category || (object)base.Reader.NamespaceURI != id2_Item)
				{
					throw CreateUnknownNodeException();
				}
				result = Read4_Category(isNullable: true, checkType: true);
			}
			else
			{
				UnknownNode(null, ":Category");
			}
			return result;
		}

		private KSPediaDatabase.Category Read4_Category(bool isNullable, bool checkType)
		{
			XmlQualifiedName xmlQualifiedName = (checkType ? GetXsiType() : null);
			bool flag = false;
			if (isNullable)
			{
				flag = ReadNull();
			}
			if (checkType && !(xmlQualifiedName == null) && ((object)xmlQualifiedName.Name != id7_Category || (object)xmlQualifiedName.Namespace != id2_Item))
			{
				throw CreateUnknownTypeException(xmlQualifiedName);
			}
			if (flag)
			{
				return null;
			}
			KSPediaDatabase.Category category = new KSPediaDatabase.Category();
			if (category.screens == null)
			{
				category.screens = new List<string>();
			}
			_ = category.screens;
			if (category.subcategories == null)
			{
				category.subcategories = new List<KSPediaDatabase.Category>();
			}
			_ = category.subcategories;
			bool[] array = new bool[5];
			while (base.Reader.MoveToNextAttribute())
			{
				if (!array[0] && (object)base.Reader.LocalName == id8_Name && (object)base.Reader.NamespaceURI == id2_Item)
				{
					category.name = base.Reader.Value;
					array[0] = true;
				}
				else if (!IsXmlnsAttribute(base.Reader.Name))
				{
					UnknownNode(category, ":Name");
				}
			}
			base.Reader.MoveToElement();
			if (base.Reader.IsEmptyElement)
			{
				base.Reader.Skip();
				return category;
			}
			base.Reader.ReadStartElement();
			base.Reader.MoveToContent();
			int whileIterations = 0;
			int readerCount = base.ReaderCount;
			while (base.Reader.NodeType != XmlNodeType.EndElement && base.Reader.NodeType != 0)
			{
				if (base.Reader.NodeType == XmlNodeType.Element)
				{
					if (!array[1] && (object)base.Reader.LocalName == id9_Title && (object)base.Reader.NamespaceURI == id2_Item)
					{
						category.title = base.Reader.ReadElementString();
						array[1] = true;
					}
					else if (!array[2] && (object)base.Reader.LocalName == id10_TitleScreen && (object)base.Reader.NamespaceURI == id2_Item)
					{
						category.titleScreen = base.Reader.ReadElementString();
						array[2] = true;
					}
					else if ((object)base.Reader.LocalName == id11_Screens && (object)base.Reader.NamespaceURI == id2_Item)
					{
						if (!ReadNull())
						{
							if (category.screens == null)
							{
								category.screens = new List<string>();
							}
							List<string> screens = category.screens;
							if (base.Reader.IsEmptyElement)
							{
								base.Reader.Skip();
							}
							else
							{
								base.Reader.ReadStartElement();
								base.Reader.MoveToContent();
								int whileIterations2 = 0;
								int readerCount2 = base.ReaderCount;
								while (base.Reader.NodeType != XmlNodeType.EndElement && base.Reader.NodeType != 0)
								{
									if (base.Reader.NodeType == XmlNodeType.Element)
									{
										if ((object)base.Reader.LocalName == id5_Screen && (object)base.Reader.NamespaceURI == id2_Item)
										{
											if (ReadNull())
											{
												screens.Add(null);
											}
											else
											{
												screens.Add(base.Reader.ReadElementString());
											}
										}
										else
										{
											UnknownNode(null, ":Screen");
										}
									}
									else
									{
										UnknownNode(null, ":Screen");
									}
									base.Reader.MoveToContent();
									CheckReaderCount(ref whileIterations2, ref readerCount2);
								}
								ReadEndElement();
							}
						}
					}
					else if ((object)base.Reader.LocalName == id12_Subcategories && (object)base.Reader.NamespaceURI == id2_Item)
					{
						if (!ReadNull())
						{
							if (category.subcategories == null)
							{
								category.subcategories = new List<KSPediaDatabase.Category>();
							}
							List<KSPediaDatabase.Category> subcategories = category.subcategories;
							if (base.Reader.IsEmptyElement)
							{
								base.Reader.Skip();
							}
							else
							{
								base.Reader.ReadStartElement();
								base.Reader.MoveToContent();
								int whileIterations3 = 0;
								int readerCount3 = base.ReaderCount;
								while (base.Reader.NodeType != XmlNodeType.EndElement && base.Reader.NodeType != 0)
								{
									if (base.Reader.NodeType == XmlNodeType.Element)
									{
										if ((object)base.Reader.LocalName == id13_Subcategory && (object)base.Reader.NamespaceURI == id2_Item)
										{
											if (subcategories == null)
											{
												base.Reader.Skip();
											}
											else
											{
												subcategories.Add(Read4_Category(isNullable: true, checkType: true));
											}
										}
										else
										{
											UnknownNode(null, ":Subcategory");
										}
									}
									else
									{
										UnknownNode(null, ":Subcategory");
									}
									base.Reader.MoveToContent();
									CheckReaderCount(ref whileIterations3, ref readerCount3);
								}
								ReadEndElement();
							}
						}
					}
					else
					{
						UnknownNode(category, ":Title, :TitleScreen, :Screens, :Subcategories");
					}
				}
				else
				{
					UnknownNode(category, ":Title, :TitleScreen, :Screens, :Subcategories");
				}
				base.Reader.MoveToContent();
				CheckReaderCount(ref whileIterations, ref readerCount);
			}
			ReadEndElement();
			return category;
		}

		private KSPediaDatabase.ScreenText Read5_ScreenText(bool isNullable, bool checkType)
		{
			XmlQualifiedName xmlQualifiedName = (checkType ? GetXsiType() : null);
			bool flag = false;
			if (isNullable)
			{
				flag = ReadNull();
			}
			if (checkType && !(xmlQualifiedName == null) && ((object)xmlQualifiedName.Name != id6_ScreenText || (object)xmlQualifiedName.Namespace != id2_Item))
			{
				throw CreateUnknownTypeException(xmlQualifiedName);
			}
			if (flag)
			{
				return null;
			}
			KSPediaDatabase.ScreenText screenText = new KSPediaDatabase.ScreenText();
			bool[] array = new bool[2];
			while (base.Reader.MoveToNextAttribute())
			{
				if (!array[0] && (object)base.Reader.LocalName == id8_Name && (object)base.Reader.NamespaceURI == id2_Item)
				{
					screenText.name = base.Reader.Value;
					array[0] = true;
				}
				else if (!IsXmlnsAttribute(base.Reader.Name))
				{
					UnknownNode(screenText, ":Name");
				}
			}
			base.Reader.MoveToElement();
			if (base.Reader.IsEmptyElement)
			{
				base.Reader.Skip();
				return screenText;
			}
			base.Reader.ReadStartElement();
			base.Reader.MoveToContent();
			int whileIterations = 0;
			int readerCount = base.ReaderCount;
			while (base.Reader.NodeType != XmlNodeType.EndElement && base.Reader.NodeType != 0)
			{
				if (base.Reader.NodeType == XmlNodeType.Element)
				{
					if (!array[1] && (object)base.Reader.LocalName == id14_Text && (object)base.Reader.NamespaceURI == id2_Item)
					{
						screenText.text = base.Reader.ReadElementString();
						array[1] = true;
					}
					else
					{
						UnknownNode(screenText, ":Text");
					}
				}
				else
				{
					UnknownNode(screenText, ":Text");
				}
				base.Reader.MoveToContent();
				CheckReaderCount(ref whileIterations, ref readerCount);
			}
			ReadEndElement();
			return screenText;
		}

		private KSPediaDatabase.Screen Read6_Screen(bool isNullable, bool checkType)
		{
			XmlQualifiedName xmlQualifiedName = (checkType ? GetXsiType() : null);
			bool flag = false;
			if (isNullable)
			{
				flag = ReadNull();
			}
			if (checkType && !(xmlQualifiedName == null) && ((object)xmlQualifiedName.Name != id5_Screen || (object)xmlQualifiedName.Namespace != id2_Item))
			{
				throw CreateUnknownTypeException(xmlQualifiedName);
			}
			if (flag)
			{
				return null;
			}
			KSPediaDatabase.Screen screen = new KSPediaDatabase.Screen();
			if (screen.text == null)
			{
				screen.text = new List<KSPediaDatabase.ScreenText>();
			}
			_ = screen.text;
			if (screen.tooltips == null)
			{
				screen.tooltips = new List<KSPediaDatabase.ScreenText>();
			}
			_ = screen.tooltips;
			bool[] array = new bool[7];
			while (base.Reader.MoveToNextAttribute())
			{
				if (!array[0] && (object)base.Reader.LocalName == id8_Name && (object)base.Reader.NamespaceURI == id2_Item)
				{
					screen.name = base.Reader.Value;
					array[0] = true;
				}
				else if (!IsXmlnsAttribute(base.Reader.Name))
				{
					UnknownNode(screen, ":Name");
				}
			}
			base.Reader.MoveToElement();
			if (base.Reader.IsEmptyElement)
			{
				base.Reader.Skip();
				return screen;
			}
			base.Reader.ReadStartElement();
			base.Reader.MoveToContent();
			int whileIterations = 0;
			int readerCount = base.ReaderCount;
			while (base.Reader.NodeType != XmlNodeType.EndElement && base.Reader.NodeType != 0)
			{
				if (base.Reader.NodeType == XmlNodeType.Element)
				{
					if (!array[1] && (object)base.Reader.LocalName == id15_BundleName && (object)base.Reader.NamespaceURI == id2_Item)
					{
						screen.bundleName = base.Reader.ReadElementString();
						array[1] = true;
					}
					else if (!array[2] && (object)base.Reader.LocalName == id16_AssetPath && (object)base.Reader.NamespaceURI == id2_Item)
					{
						screen.assetPath = base.Reader.ReadElementString();
						array[2] = true;
					}
					else if (!array[3] && (object)base.Reader.LocalName == id9_Title && (object)base.Reader.NamespaceURI == id2_Item)
					{
						screen.title = base.Reader.ReadElementString();
						array[3] = true;
					}
					else if (!array[4] && (object)base.Reader.LocalName == id17_Background && (object)base.Reader.NamespaceURI == id2_Item)
					{
						screen.background = base.Reader.ReadElementString();
						array[4] = true;
					}
					else if ((object)base.Reader.LocalName == id14_Text && (object)base.Reader.NamespaceURI == id2_Item)
					{
						if (!ReadNull())
						{
							if (screen.text == null)
							{
								screen.text = new List<KSPediaDatabase.ScreenText>();
							}
							List<KSPediaDatabase.ScreenText> text = screen.text;
							if (base.Reader.IsEmptyElement)
							{
								base.Reader.Skip();
							}
							else
							{
								base.Reader.ReadStartElement();
								base.Reader.MoveToContent();
								int whileIterations2 = 0;
								int readerCount2 = base.ReaderCount;
								while (base.Reader.NodeType != XmlNodeType.EndElement && base.Reader.NodeType != 0)
								{
									if (base.Reader.NodeType == XmlNodeType.Element)
									{
										if ((object)base.Reader.LocalName == id14_Text && (object)base.Reader.NamespaceURI == id2_Item)
										{
											if (text == null)
											{
												base.Reader.Skip();
											}
											else
											{
												text.Add(Read5_ScreenText(isNullable: true, checkType: true));
											}
										}
										else
										{
											UnknownNode(null, ":Text");
										}
									}
									else
									{
										UnknownNode(null, ":Text");
									}
									base.Reader.MoveToContent();
									CheckReaderCount(ref whileIterations2, ref readerCount2);
								}
								ReadEndElement();
							}
						}
					}
					else if ((object)base.Reader.LocalName == id18_Tooltips && (object)base.Reader.NamespaceURI == id2_Item)
					{
						if (!ReadNull())
						{
							if (screen.tooltips == null)
							{
								screen.tooltips = new List<KSPediaDatabase.ScreenText>();
							}
							List<KSPediaDatabase.ScreenText> tooltips = screen.tooltips;
							if (base.Reader.IsEmptyElement)
							{
								base.Reader.Skip();
							}
							else
							{
								base.Reader.ReadStartElement();
								base.Reader.MoveToContent();
								int whileIterations3 = 0;
								int readerCount3 = base.ReaderCount;
								while (base.Reader.NodeType != XmlNodeType.EndElement && base.Reader.NodeType != 0)
								{
									if (base.Reader.NodeType == XmlNodeType.Element)
									{
										if ((object)base.Reader.LocalName == id19_Tooltip && (object)base.Reader.NamespaceURI == id2_Item)
										{
											if (tooltips == null)
											{
												base.Reader.Skip();
											}
											else
											{
												tooltips.Add(Read5_ScreenText(isNullable: true, checkType: true));
											}
										}
										else
										{
											UnknownNode(null, ":Tooltip");
										}
									}
									else
									{
										UnknownNode(null, ":Tooltip");
									}
									base.Reader.MoveToContent();
									CheckReaderCount(ref whileIterations3, ref readerCount3);
								}
								ReadEndElement();
							}
						}
					}
					else
					{
						UnknownNode(screen, ":BundleName, :AssetPath, :Title, :Background, :Text, :Tooltips");
					}
				}
				else
				{
					UnknownNode(screen, ":BundleName, :AssetPath, :Title, :Background, :Text, :Tooltips");
				}
				base.Reader.MoveToContent();
				CheckReaderCount(ref whileIterations, ref readerCount);
			}
			ReadEndElement();
			return screen;
		}

		private KSPediaDatabase Read7_KSPediaDatabase(bool isNullable, bool checkType)
		{
			XmlQualifiedName xmlQualifiedName = (checkType ? GetXsiType() : null);
			bool flag = false;
			if (isNullable)
			{
				flag = ReadNull();
			}
			if (checkType && !(xmlQualifiedName == null) && ((object)xmlQualifiedName.Name != id20_KSPediaDatabase || (object)xmlQualifiedName.Namespace != id2_Item))
			{
				throw CreateUnknownTypeException(xmlQualifiedName);
			}
			if (flag)
			{
				return null;
			}
			KSPediaDatabase kSPediaDatabase = new KSPediaDatabase();
			if (kSPediaDatabase.categories == null)
			{
				kSPediaDatabase.categories = new List<KSPediaDatabase.Category>();
			}
			_ = kSPediaDatabase.categories;
			if (kSPediaDatabase.screens == null)
			{
				kSPediaDatabase.screens = new List<KSPediaDatabase.Screen>();
			}
			_ = kSPediaDatabase.screens;
			bool[] array = new bool[4];
			while (base.Reader.MoveToNextAttribute())
			{
				if (!array[0] && (object)base.Reader.LocalName == id8_Name && (object)base.Reader.NamespaceURI == id2_Item)
				{
					kSPediaDatabase.name = base.Reader.Value;
					array[0] = true;
				}
				else if (!array[1] && (object)base.Reader.LocalName == id21_CreatedTime && (object)base.Reader.NamespaceURI == id2_Item)
				{
					kSPediaDatabase.createdTime = XmlConvert.ToInt64(base.Reader.Value);
					array[1] = true;
				}
				else if (!IsXmlnsAttribute(base.Reader.Name))
				{
					UnknownNode(kSPediaDatabase, ":Name, :CreatedTime");
				}
			}
			base.Reader.MoveToElement();
			if (base.Reader.IsEmptyElement)
			{
				base.Reader.Skip();
				return kSPediaDatabase;
			}
			base.Reader.ReadStartElement();
			base.Reader.MoveToContent();
			int whileIterations = 0;
			int readerCount = base.ReaderCount;
			while (base.Reader.NodeType != XmlNodeType.EndElement && base.Reader.NodeType != 0)
			{
				if (base.Reader.NodeType == XmlNodeType.Element)
				{
					if ((object)base.Reader.LocalName == id22_Categories && (object)base.Reader.NamespaceURI == id2_Item)
					{
						if (!ReadNull())
						{
							if (kSPediaDatabase.categories == null)
							{
								kSPediaDatabase.categories = new List<KSPediaDatabase.Category>();
							}
							List<KSPediaDatabase.Category> categories = kSPediaDatabase.categories;
							if (base.Reader.IsEmptyElement)
							{
								base.Reader.Skip();
							}
							else
							{
								base.Reader.ReadStartElement();
								base.Reader.MoveToContent();
								int whileIterations2 = 0;
								int readerCount2 = base.ReaderCount;
								while (base.Reader.NodeType != XmlNodeType.EndElement && base.Reader.NodeType != 0)
								{
									if (base.Reader.NodeType == XmlNodeType.Element)
									{
										if ((object)base.Reader.LocalName == id7_Category && (object)base.Reader.NamespaceURI == id2_Item)
										{
											if (categories == null)
											{
												base.Reader.Skip();
											}
											else
											{
												categories.Add(Read4_Category(isNullable: true, checkType: true));
											}
										}
										else
										{
											UnknownNode(null, ":Category");
										}
									}
									else
									{
										UnknownNode(null, ":Category");
									}
									base.Reader.MoveToContent();
									CheckReaderCount(ref whileIterations2, ref readerCount2);
								}
								ReadEndElement();
							}
						}
					}
					else if ((object)base.Reader.LocalName == id11_Screens && (object)base.Reader.NamespaceURI == id2_Item)
					{
						if (!ReadNull())
						{
							if (kSPediaDatabase.screens == null)
							{
								kSPediaDatabase.screens = new List<KSPediaDatabase.Screen>();
							}
							List<KSPediaDatabase.Screen> screens = kSPediaDatabase.screens;
							if (base.Reader.IsEmptyElement)
							{
								base.Reader.Skip();
							}
							else
							{
								base.Reader.ReadStartElement();
								base.Reader.MoveToContent();
								int whileIterations3 = 0;
								int readerCount3 = base.ReaderCount;
								while (base.Reader.NodeType != XmlNodeType.EndElement && base.Reader.NodeType != 0)
								{
									if (base.Reader.NodeType == XmlNodeType.Element)
									{
										if ((object)base.Reader.LocalName == id5_Screen && (object)base.Reader.NamespaceURI == id2_Item)
										{
											if (screens == null)
											{
												base.Reader.Skip();
											}
											else
											{
												screens.Add(Read6_Screen(isNullable: true, checkType: true));
											}
										}
										else
										{
											UnknownNode(null, ":Screen");
										}
									}
									else
									{
										UnknownNode(null, ":Screen");
									}
									base.Reader.MoveToContent();
									CheckReaderCount(ref whileIterations3, ref readerCount3);
								}
								ReadEndElement();
							}
						}
					}
					else
					{
						UnknownNode(kSPediaDatabase, ":Categories, :Screens");
					}
				}
				else
				{
					UnknownNode(kSPediaDatabase, ":Categories, :Screens");
				}
				base.Reader.MoveToContent();
				CheckReaderCount(ref whileIterations, ref readerCount);
			}
			ReadEndElement();
			return kSPediaDatabase;
		}

		private BundleDefinition Read3_BundleDefinition(bool isNullable, bool checkType)
		{
			XmlQualifiedName xmlQualifiedName = (checkType ? GetXsiType() : null);
			bool flag = false;
			if (isNullable)
			{
				flag = ReadNull();
			}
			if (checkType && !(xmlQualifiedName == null) && ((object)xmlQualifiedName.Name != id23_BundleDefinition || (object)xmlQualifiedName.Namespace != id2_Item))
			{
				throw CreateUnknownTypeException(xmlQualifiedName);
			}
			if (flag)
			{
				return null;
			}
			BundleDefinition bundleDefinition = new BundleDefinition();
			if (bundleDefinition.assets == null)
			{
				bundleDefinition.assets = new List<AssetDefinition>();
			}
			_ = bundleDefinition.assets;
			if (bundleDefinition.dependencyNames == null)
			{
				bundleDefinition.dependencyNames = new List<string>();
			}
			_ = bundleDefinition.dependencyNames;
			bool[] array = new bool[7];
			while (base.Reader.MoveToNextAttribute())
			{
				if (!array[0] && (object)base.Reader.LocalName == id24_UrlName && (object)base.Reader.NamespaceURI == id2_Item)
				{
					bundleDefinition.urlName = base.Reader.Value;
					array[0] = true;
				}
				else if (!array[1] && (object)base.Reader.LocalName == id8_Name && (object)base.Reader.NamespaceURI == id2_Item)
				{
					bundleDefinition.name = base.Reader.Value;
					array[1] = true;
				}
				else if (!array[2] && (object)base.Reader.LocalName == id21_CreatedTime && (object)base.Reader.NamespaceURI == id2_Item)
				{
					bundleDefinition.createdTime = XmlConvert.ToInt64(base.Reader.Value);
					array[2] = true;
				}
				else if (!IsXmlnsAttribute(base.Reader.Name))
				{
					UnknownNode(bundleDefinition, ":UrlName, :Name, :CreatedTime");
				}
			}
			base.Reader.MoveToElement();
			if (base.Reader.IsEmptyElement)
			{
				base.Reader.Skip();
				return bundleDefinition;
			}
			base.Reader.ReadStartElement();
			base.Reader.MoveToContent();
			int whileIterations = 0;
			int readerCount = base.ReaderCount;
			while (base.Reader.NodeType != XmlNodeType.EndElement && base.Reader.NodeType != 0)
			{
				if (base.Reader.NodeType == XmlNodeType.Element)
				{
					if (!array[3] && (object)base.Reader.LocalName == id25_Author && (object)base.Reader.NamespaceURI == id2_Item)
					{
						bundleDefinition.author = base.Reader.ReadElementString();
						array[3] = true;
					}
					else if (!array[4] && (object)base.Reader.LocalName == id26_Info && (object)base.Reader.NamespaceURI == id2_Item)
					{
						bundleDefinition.info = base.Reader.ReadElementString();
						array[4] = true;
					}
					else if ((object)base.Reader.LocalName == id27_Assets && (object)base.Reader.NamespaceURI == id2_Item)
					{
						if (!ReadNull())
						{
							if (bundleDefinition.assets == null)
							{
								bundleDefinition.assets = new List<AssetDefinition>();
							}
							List<AssetDefinition> assets = bundleDefinition.assets;
							if (base.Reader.IsEmptyElement)
							{
								base.Reader.Skip();
							}
							else
							{
								base.Reader.ReadStartElement();
								base.Reader.MoveToContent();
								int whileIterations2 = 0;
								int readerCount2 = base.ReaderCount;
								while (base.Reader.NodeType != XmlNodeType.EndElement && base.Reader.NodeType != 0)
								{
									if (base.Reader.NodeType == XmlNodeType.Element)
									{
										if ((object)base.Reader.LocalName == id28_Asset && (object)base.Reader.NamespaceURI == id2_Item)
										{
											if (assets == null)
											{
												base.Reader.Skip();
											}
											else
											{
												assets.Add(Read2_AssetDefinition(isNullable: true, checkType: true));
											}
										}
										else
										{
											UnknownNode(null, ":Asset");
										}
									}
									else
									{
										UnknownNode(null, ":Asset");
									}
									base.Reader.MoveToContent();
									CheckReaderCount(ref whileIterations2, ref readerCount2);
								}
								ReadEndElement();
							}
						}
					}
					else if ((object)base.Reader.LocalName == id29_Dependencies && (object)base.Reader.NamespaceURI == id2_Item)
					{
						if (!ReadNull())
						{
							if (bundleDefinition.dependencyNames == null)
							{
								bundleDefinition.dependencyNames = new List<string>();
							}
							List<string> dependencyNames = bundleDefinition.dependencyNames;
							if (base.Reader.IsEmptyElement)
							{
								base.Reader.Skip();
							}
							else
							{
								base.Reader.ReadStartElement();
								base.Reader.MoveToContent();
								int whileIterations3 = 0;
								int readerCount3 = base.ReaderCount;
								while (base.Reader.NodeType != XmlNodeType.EndElement && base.Reader.NodeType != 0)
								{
									if (base.Reader.NodeType == XmlNodeType.Element)
									{
										if ((object)base.Reader.LocalName == id30_Dependency && (object)base.Reader.NamespaceURI == id2_Item)
										{
											if (ReadNull())
											{
												dependencyNames.Add(null);
											}
											else
											{
												dependencyNames.Add(base.Reader.ReadElementString());
											}
										}
										else
										{
											UnknownNode(null, ":Dependency");
										}
									}
									else
									{
										UnknownNode(null, ":Dependency");
									}
									base.Reader.MoveToContent();
									CheckReaderCount(ref whileIterations3, ref readerCount3);
								}
								ReadEndElement();
							}
						}
					}
					else
					{
						UnknownNode(bundleDefinition, ":Author, :Info, :Assets, :Dependencies");
					}
				}
				else
				{
					UnknownNode(bundleDefinition, ":Author, :Info, :Assets, :Dependencies");
				}
				base.Reader.MoveToContent();
				CheckReaderCount(ref whileIterations, ref readerCount);
			}
			ReadEndElement();
			return bundleDefinition;
		}

		private AssetDefinition Read2_AssetDefinition(bool isNullable, bool checkType)
		{
			XmlQualifiedName xmlQualifiedName = (checkType ? GetXsiType() : null);
			bool flag = false;
			if (isNullable)
			{
				flag = ReadNull();
			}
			if (checkType && !(xmlQualifiedName == null) && ((object)xmlQualifiedName.Name != id1_AssetDefinition || (object)xmlQualifiedName.Namespace != id2_Item))
			{
				throw CreateUnknownTypeException(xmlQualifiedName);
			}
			if (flag)
			{
				return null;
			}
			AssetDefinition assetDefinition = new AssetDefinition();
			bool[] array = new bool[5];
			while (base.Reader.MoveToNextAttribute())
			{
				if (!array[0] && (object)base.Reader.LocalName == id8_Name && (object)base.Reader.NamespaceURI == id2_Item)
				{
					assetDefinition.name = base.Reader.Value;
					array[0] = true;
				}
				else if (!array[1] && (object)base.Reader.LocalName == id31_Path && (object)base.Reader.NamespaceURI == id2_Item)
				{
					assetDefinition.path = base.Reader.Value;
					array[1] = true;
				}
				else if (!array[2] && (object)base.Reader.LocalName == id32_Type && (object)base.Reader.NamespaceURI == id2_Item)
				{
					assetDefinition.type = base.Reader.Value;
					array[2] = true;
				}
				else if (!array[3] && (object)base.Reader.LocalName == id33_Url && (object)base.Reader.NamespaceURI == id2_Item)
				{
					assetDefinition.url = base.Reader.Value;
					array[3] = true;
				}
				else if (!array[4] && (object)base.Reader.LocalName == id34_AutoLoad && (object)base.Reader.NamespaceURI == id2_Item)
				{
					assetDefinition.autoLoad = XmlConvert.ToBoolean(base.Reader.Value);
					array[4] = true;
				}
				else if (!IsXmlnsAttribute(base.Reader.Name))
				{
					UnknownNode(assetDefinition, ":Name, :Path, :Type, :Url, :AutoLoad");
				}
			}
			base.Reader.MoveToElement();
			if (base.Reader.IsEmptyElement)
			{
				base.Reader.Skip();
				return assetDefinition;
			}
			base.Reader.ReadStartElement();
			base.Reader.MoveToContent();
			int whileIterations = 0;
			int readerCount = base.ReaderCount;
			while (base.Reader.NodeType != XmlNodeType.EndElement && base.Reader.NodeType != 0)
			{
				if (base.Reader.NodeType == XmlNodeType.Element)
				{
					UnknownNode(assetDefinition, "");
				}
				else
				{
					UnknownNode(assetDefinition, "");
				}
				base.Reader.MoveToContent();
				CheckReaderCount(ref whileIterations, ref readerCount);
			}
			ReadEndElement();
			return assetDefinition;
		}

		protected override void InitCallbacks()
		{
		}

		protected override void InitIDs()
		{
			id21_CreatedTime = base.Reader.NameTable.Add("CreatedTime");
			id34_AutoLoad = base.Reader.NameTable.Add("AutoLoad");
			id6_ScreenText = base.Reader.NameTable.Add("ScreenText");
			id22_Categories = base.Reader.NameTable.Add("Categories");
			id26_Info = base.Reader.NameTable.Add("Info");
			id5_Screen = base.Reader.NameTable.Add("Screen");
			id18_Tooltips = base.Reader.NameTable.Add("Tooltips");
			id11_Screens = base.Reader.NameTable.Add("Screens");
			id29_Dependencies = base.Reader.NameTable.Add("Dependencies");
			id7_Category = base.Reader.NameTable.Add("Category");
			id3_KSPBundleDefinition = base.Reader.NameTable.Add("KSPBundleDefinition");
			id23_BundleDefinition = base.Reader.NameTable.Add("BundleDefinition");
			id30_Dependency = base.Reader.NameTable.Add("Dependency");
			id31_Path = base.Reader.NameTable.Add("Path");
			id28_Asset = base.Reader.NameTable.Add("Asset");
			id1_AssetDefinition = base.Reader.NameTable.Add("AssetDefinition");
			id2_Item = base.Reader.NameTable.Add("");
			id10_TitleScreen = base.Reader.NameTable.Add("TitleScreen");
			id13_Subcategory = base.Reader.NameTable.Add("Subcategory");
			id25_Author = base.Reader.NameTable.Add("Author");
			id16_AssetPath = base.Reader.NameTable.Add("AssetPath");
			id8_Name = base.Reader.NameTable.Add("Name");
			id24_UrlName = base.Reader.NameTable.Add("UrlName");
			id33_Url = base.Reader.NameTable.Add("Url");
			id4_KSPedia = base.Reader.NameTable.Add("KSPedia");
			id19_Tooltip = base.Reader.NameTable.Add("Tooltip");
			id27_Assets = base.Reader.NameTable.Add("Assets");
			id15_BundleName = base.Reader.NameTable.Add("BundleName");
			id17_Background = base.Reader.NameTable.Add("Background");
			id32_Type = base.Reader.NameTable.Add("Type");
			id12_Subcategories = base.Reader.NameTable.Add("Subcategories");
			id20_KSPediaDatabase = base.Reader.NameTable.Add("KSPediaDatabase");
			id14_Text = base.Reader.NameTable.Add("Text");
			id9_Title = base.Reader.NameTable.Add("Title");
		}
	}
}

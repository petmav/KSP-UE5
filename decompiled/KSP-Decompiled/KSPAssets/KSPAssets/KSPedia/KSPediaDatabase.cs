using System;
using System.Collections.Generic;
using System.IO;
using System.Xml.Serialization;
using UnityEngine;
using UnityEngine.UI;

namespace KSPAssets.KSPedia
{
	[Serializable]
	[XmlRoot("KSPedia")]
	public class KSPediaDatabase
	{
		[Serializable]
		public class Screen
		{
			[XmlAttribute("Name")]
			public string name = "";

			[XmlElement("BundleName")]
			public string bundleName = "";

			[XmlElement("AssetPath")]
			public string assetPath = "";

			[XmlElement("Title")]
			public string title = "";

			[XmlElement("Background")]
			public string background = "";

			[XmlArray("Text")]
			[XmlArrayItem("Text")]
			public List<ScreenText> text = new List<ScreenText>();

			[XmlArray("Tooltips")]
			[XmlArrayItem("Tooltip")]
			public List<ScreenText> tooltips = new List<ScreenText>();

			public Screen()
			{
			}

			public Screen(DatabaseScreen screen)
			{
				bundleName = screen.bundleName;
				assetPath = screen.assetPath;
				name = screen.screenName;
				title = screen.title;
				CompileText(screen);
				CompileTooltips(screen);
			}

			private void CompileText(DatabaseScreen screen)
			{
				text.Clear();
				Text[] componentsInChildren = screen.GetComponentsInChildren<Text>(includeInactive: true);
				for (int i = 0; i < componentsInChildren.Length; i++)
				{
					text.Add(new ScreenText(GetComponentURL(screen.transform, componentsInChildren[i].transform), componentsInChildren[i].text));
				}
			}

			private void CompileTooltips(DatabaseScreen screen)
			{
				tooltips.Clear();
				DatabaseTooltip[] componentsInChildren = screen.GetComponentsInChildren<DatabaseTooltip>(includeInactive: true);
				for (int i = 0; i < componentsInChildren.Length; i++)
				{
					tooltips.Add(new ScreenText(GetComponentURL(screen.transform, componentsInChildren[i].transform), componentsInChildren[i].text));
				}
			}

			public static string GetComponentURL(Transform root, Transform t)
			{
				if (t == root)
				{
					return "";
				}
				string componentURL = GetComponentURL(root, t.parent);
				if (string.IsNullOrEmpty(componentURL))
				{
					return t.gameObject.name;
				}
				return componentURL + "/" + t.gameObject.name;
			}

			public ScreenText GetText(string name)
			{
				for (int i = 0; i < text.Count; i++)
				{
					if (text[i].name == name)
					{
						return text[i];
					}
				}
				return null;
			}

			public ScreenText GetTooltip(string name)
			{
				for (int i = 0; i < tooltips.Count; i++)
				{
					if (tooltips[i].name == name)
					{
						return tooltips[i];
					}
				}
				return null;
			}
		}

		[Serializable]
		public class ScreenText
		{
			[XmlAttribute("Name")]
			public string name = "";

			[XmlElement("Text")]
			public string text = "";

			public ScreenText()
			{
			}

			public ScreenText(string name, string text)
			{
				this.name = name;
				this.text = text;
			}
		}

		public class Category
		{
			[XmlAttribute("Name")]
			public string name = "";

			[XmlElement("Title")]
			public string title = "";

			[XmlElement("TitleScreen")]
			public string titleScreen = "";

			[XmlArray("Screens")]
			[XmlArrayItem("Screen")]
			public List<string> screens = new List<string>();

			[XmlArray("Subcategories")]
			[XmlArrayItem("Subcategory")]
			public List<Category> subcategories = new List<Category>();

			public Category()
			{
			}

			public Category(string name, string subcategory = null)
			{
				this.name = name;
				title = this.name;
				if (!string.IsNullOrEmpty(subcategory))
				{
					subcategories.Add(new Category(subcategory));
				}
			}
		}

		[XmlAttribute("Name")]
		public string name = "";

		[XmlAttribute("CreatedTime")]
		public long createdTime = -1L;

		[XmlArray("Categories")]
		[XmlArrayItem("Category")]
		public List<Category> categories = new List<Category>();

		[XmlArray("Screens")]
		[XmlArrayItem("Screen")]
		public List<Screen> screens = new List<Screen>();

		public KSPediaDatabase()
		{
			createdTime = DateTime.Now.ToBinary();
		}

		public KSPediaDatabase(string name)
		{
			this.name = name;
			createdTime = DateTime.Now.ToBinary();
		}

		public void Save(string path)
		{
			using (StreamWriter streamWriter = new StreamWriter(path))
			{
				XmlSerializer xmlSerializer = new XmlSerializer(GetType());
				xmlSerializer.Serialize(streamWriter, this);
				streamWriter.Flush();
			}
		}

		public static KSPediaDatabase LoadFromFile(string path)
		{
			using (FileStream stream = File.OpenRead(path))
			{
				XmlSerializer xmlSerializer = new XmlSerializer(typeof(KSPediaDatabase));
				return xmlSerializer.Deserialize(stream) as KSPediaDatabase;
			}
		}

		public static KSPediaDatabase CreateFromText(string text)
		{
			using (MemoryStream stream = GenerateStreamFromString(text))
			{
				XmlSerializer xmlSerializer = new XmlSerializer(typeof(KSPediaDatabase));
				return xmlSerializer.Deserialize(stream) as KSPediaDatabase;
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

		public void ClearScreens()
		{
			screens.Clear();
		}

		public bool AddScreen(DatabaseScreen dbScreen)
		{
			Screen screen = GetScreen(dbScreen);
			if (screen != null)
			{
				Debug.LogError("Database: Cannot AddScreen - Screen with name '" + dbScreen.screenName + "' already exists!");
				return false;
			}
			screens.Add(new Screen(dbScreen));
			return true;
		}

		public Screen GetScreen(DatabaseScreen dbScreen)
		{
			return GetScreen(dbScreen.screenName);
		}

		public Screen GetScreen(string name)
		{
			if (string.IsNullOrEmpty(name))
			{
				return null;
			}
			for (int i = 0; i < screens.Count; i++)
			{
				if (screens[i].name == name)
				{
					return screens[i];
				}
			}
			return null;
		}

		public int GetScreenIndex(string name)
		{
			Screen screen = GetScreen(name);
			if (screen == null)
			{
				return -1;
			}
			return screens.IndexOf(screen);
		}

		public Screen[] GetScreens(string category, string subcategory = null)
		{
			Category category2 = GetCategory(category, subcategory);
			if (category2 == null)
			{
				return new Screen[0];
			}
			List<Screen> list = new List<Screen>();
			for (int i = 0; i < category2.screens.Count; i++)
			{
				list.Add(screens[i]);
			}
			return list.ToArray();
		}

		public Screen[] GetScreens(Category cat, bool includeAllSubcategoryScreens = false)
		{
			if (IsRootCategory(cat))
			{
				if (includeAllSubcategoryScreens)
				{
					List<Screen> list = new List<Screen>();
					list.AddRange(GetScreens(cat.name));
					for (int i = 0; i < cat.subcategories.Count; i++)
					{
						list.AddRange(GetScreens(cat.name, cat.subcategories[i].name));
					}
					return list.ToArray();
				}
				return GetScreens(cat.name);
			}
			return GetScreens(GetRootCategory(cat).name, cat.name);
		}

		public Screen[] GetScreens(bool assigned, bool unassigned)
		{
			if (assigned && unassigned)
			{
				return screens.ToArray();
			}
			List<Screen> list = new List<Screen>();
			if (assigned || unassigned)
			{
				for (int i = 0; i < categories.Count; i++)
				{
					Category category = categories[i];
					Screen screen = GetScreen(category.titleScreen);
					if (screen != null)
					{
						list.Add(screen);
					}
					for (int j = 0; j < category.screens.Count; j++)
					{
						screen = GetScreen(category.screens[j]);
						if (screen != null)
						{
							list.Add(screen);
						}
					}
					for (int k = 0; k < category.subcategories.Count; k++)
					{
						Category category2 = category.subcategories[k];
						screen = GetScreen(category2.titleScreen);
						if (screen != null)
						{
							list.Add(screen);
						}
						for (int l = 0; l < category2.screens.Count; l++)
						{
							screen = GetScreen(category2.screens[l]);
							if (screen != null)
							{
								list.Add(screen);
							}
						}
					}
				}
			}
			if (!unassigned)
			{
				return list.ToArray();
			}
			List<Screen> list2 = new List<Screen>();
			for (int m = 0; m < screens.Count; m++)
			{
				Screen screen = screens[m];
				if (list.Contains(screen))
				{
					list2.Add(screen);
				}
			}
			return list2.ToArray();
		}

		public bool IsTitleScreen(Screen screen)
		{
			for (int i = 0; i < categories.Count; i++)
			{
				if (categories[i].titleScreen == screen.name)
				{
					return true;
				}
				for (int j = 0; j < categories[i].subcategories.Count; j++)
				{
					if (categories[i].subcategories[j].titleScreen == screen.name)
					{
						return true;
					}
				}
			}
			return false;
		}

		public Screen GetScreenUp(Screen screen)
		{
			string text = screen.name;
			Category category = GetCategory(screen);
			if (category.titleScreen == text)
			{
				if (!IsRootCategory(category))
				{
					Category rootCategory = GetRootCategory(category);
					if (!string.IsNullOrEmpty(rootCategory.titleScreen))
					{
						return GetScreen(rootCategory.titleScreen);
					}
					if (rootCategory.screens.Count > 0)
					{
						return GetScreen(rootCategory.screens[0]);
					}
				}
			}
			else
			{
				if (!string.IsNullOrEmpty(category.titleScreen))
				{
					return GetScreen(category.titleScreen);
				}
				if (!IsRootCategory(category))
				{
					Category rootCategory2 = GetRootCategory(category);
					if (!string.IsNullOrEmpty(rootCategory2.titleScreen))
					{
						return GetScreen(rootCategory2.titleScreen);
					}
					if (rootCategory2.screens.Count > 0)
					{
						return GetScreen(rootCategory2.screens[0]);
					}
				}
			}
			return null;
		}

		public string GetScreenUrl(Screen screen)
		{
			Category category = GetCategory(screen);
			if (category.titleScreen == screen.name)
			{
				return GetCategoryURL(category);
			}
			return GetCategoryURL(category) + "/" + screen.name;
		}

		public Category AddCategory(string name, string subcategory = null)
		{
			Category category = GetCategory(name);
			if (category == null)
			{
				category = new Category(name, subcategory);
				categories.Add(category);
				return category;
			}
			if (string.IsNullOrEmpty(subcategory))
			{
				return null;
			}
			for (int i = 0; i < category.subcategories.Count; i++)
			{
				if (category.subcategories[i].name == subcategory)
				{
					return null;
				}
			}
			Category category2 = new Category(subcategory);
			category.subcategories.Add(category2);
			return category2;
		}

		public Category GetCategory(string name, string subcategory = null)
		{
			Category category = null;
			for (int i = 0; i < categories.Count; i++)
			{
				if (categories[i].name == name)
				{
					category = categories[i];
					break;
				}
			}
			if (category != null)
			{
				if (string.IsNullOrEmpty(subcategory))
				{
					return category;
				}
				for (int j = 0; j < category.subcategories.Count; j++)
				{
					if (category.subcategories[j].name == subcategory)
					{
						return category.subcategories[j];
					}
				}
			}
			return null;
		}

		public Category GetCategory(Screen screen)
		{
			for (int i = 0; i < categories.Count; i++)
			{
				if (categories[i].titleScreen == screen.name)
				{
					return categories[i];
				}
				for (int j = 0; j < categories[i].screens.Count; j++)
				{
					if (categories[i].screens[j] == screen.name)
					{
						return categories[i];
					}
				}
				for (int k = 0; k < categories[i].subcategories.Count; k++)
				{
					if (categories[i].subcategories[k].titleScreen == screen.name)
					{
						return categories[i].subcategories[k];
					}
					for (int l = 0; l < categories[i].subcategories[k].screens.Count; l++)
					{
						if (categories[i].subcategories[k].screens[l] == screen.name)
						{
							return categories[i].subcategories[k];
						}
					}
				}
			}
			return null;
		}

		public Category GetCategory(int rootIndex, int subIndex = -1)
		{
			if (rootIndex == -1)
			{
				return null;
			}
			for (int i = 0; i < categories.Count; i++)
			{
				if (i != rootIndex)
				{
					continue;
				}
				if (subIndex == -1)
				{
					return categories[i];
				}
				for (int j = 0; j < categories[i].subcategories.Count; j++)
				{
					if (j == subIndex)
					{
						return categories[i].subcategories[j];
					}
				}
			}
			return null;
		}

		public void SetCategory(Screen screen, int rootIndex, int subIndex)
		{
			GetCategory(screen)?.screens.Remove(screen.name);
			GetCategory(rootIndex, subIndex)?.screens.Add(screen.name);
		}

		public string GetCategoryURL(Category cat)
		{
			if (cat == null)
			{
				return "Undefined";
			}
			if (IsRootCategory(cat))
			{
				return cat.name;
			}
			return GetRootCategory(cat).name + "/" + cat.name;
		}

		public string GetCategoryURL(Screen screen)
		{
			return GetCategoryURL(GetCategory(screen));
		}

		public int GetRootCategoryIndex(Screen screen)
		{
			Category category = GetCategory(screen);
			if (category == null)
			{
				return -1;
			}
			if (IsRootCategory(category))
			{
				return categories.IndexOf(category);
			}
			return categories.IndexOf(GetRootCategory(category));
		}

		public int GetSubcategoryIndex(Screen screen)
		{
			Category category = GetCategory(screen);
			if (category == null)
			{
				return -1;
			}
			if (IsRootCategory(category))
			{
				return -1;
			}
			return GetRootCategory(category).subcategories.IndexOf(category);
		}

		public string[] GetRootCategoryNames()
		{
			List<string> list = new List<string>();
			for (int i = 0; i < categories.Count; i++)
			{
				list.Add(categories[i].name);
			}
			return list.ToArray();
		}

		public string[] GetSubcategoryNames(Category rootCat)
		{
			if (!IsRootCategory(rootCat))
			{
				return null;
			}
			List<string> list = new List<string>();
			for (int i = 0; i < rootCat.subcategories.Count; i++)
			{
				list.Add(rootCat.subcategories[i].name);
			}
			return list.ToArray();
		}

		public bool IsRootCategory(Category category)
		{
			for (int i = 0; i < categories.Count; i++)
			{
				if (categories[i] == category)
				{
					return true;
				}
			}
			return false;
		}

		public bool IsSubcategory(Category category)
		{
			for (int i = 0; i < categories.Count; i++)
			{
				for (int j = 0; j < categories[i].subcategories.Count; j++)
				{
					if (categories[i].subcategories[j] == category)
					{
						return true;
					}
				}
			}
			return false;
		}

		public Category GetRootCategory(Category category)
		{
			for (int i = 0; i < categories.Count; i++)
			{
				for (int j = 0; j < categories[i].subcategories.Count; j++)
				{
					if (categories[i].subcategories[j] == category)
					{
						return categories[i];
					}
				}
			}
			return null;
		}

		public void Merge(KSPediaDatabase merge, bool verbose = false)
		{
			foreach (Screen screen2 in merge.screens)
			{
				Screen screen = GetScreen(screen2.name);
				if (screen != null)
				{
					if (!string.IsNullOrEmpty(screen2.assetPath))
					{
						if (verbose)
						{
							Debug.LogWarning("Database: Replacing screen '" + screen.name + "'");
						}
						screens.Remove(screen);
						screens.Add(screen2);
					}
					else
					{
						if (verbose)
						{
							Debug.LogWarning("Database: Merging screen '" + screen.name + "'");
						}
						MergeScreen(screen, screen2, verbose);
					}
				}
				else
				{
					if (verbose)
					{
						Debug.LogWarning("Database: Adding screen '" + screen2.name + "'");
					}
					screens.Add(screen2);
				}
			}
			foreach (Category category2 in merge.categories)
			{
				Category category = GetCategory(category2.name);
				if (category != null)
				{
					if (verbose)
					{
						Debug.LogWarning("Database: Merging category '" + category.name + "'");
					}
					MergeCategory(category, category2, verbose);
				}
				else
				{
					if (verbose)
					{
						Debug.LogWarning("Database: Adding category '" + category2.name + "'");
					}
					categories.Add(category2);
				}
			}
		}

		private void MergeScreen(Screen s, Screen mergeScreen, bool verbose = false)
		{
			if (!string.IsNullOrEmpty(mergeScreen.title))
			{
				if (verbose)
				{
					Debug.LogWarning("Database: Screen '" + mergeScreen.name + "' overloading title with '" + mergeScreen.title + "'");
				}
				s.title = mergeScreen.title;
			}
			foreach (ScreenText item in mergeScreen.text)
			{
				ScreenText text = s.GetText(item.name);
				if (text != null && !string.IsNullOrEmpty(item.text))
				{
					if (verbose)
					{
						Debug.LogWarning("Database: Screen '" + s.name + "' text '" + item.name + "' overloading with '" + item.text + "'");
					}
					text.text = item.text;
				}
			}
			foreach (ScreenText tooltip2 in mergeScreen.tooltips)
			{
				ScreenText tooltip = s.GetTooltip(tooltip2.name);
				if (tooltip != null && !string.IsNullOrEmpty(tooltip2.text))
				{
					if (verbose)
					{
						Debug.LogWarning("Database: Screen '" + s.name + "' tooltip '" + tooltip2.name + "' overloading with '" + tooltip2.text + "'");
					}
					tooltip.text = tooltip2.text;
				}
			}
		}

		private void MergeCategory(Category c, Category mergeCat, bool verbose = false)
		{
			if (!string.IsNullOrEmpty(mergeCat.title))
			{
				if (verbose)
				{
					Debug.LogWarning("Database: Category '" + mergeCat.name + "' replacing title with '" + mergeCat.title + "'");
				}
				c.title = mergeCat.title;
			}
			foreach (Category subcategory in mergeCat.subcategories)
			{
				Category category = GetCategory(mergeCat.name, subcategory.name);
				if (category != null)
				{
					if (!string.IsNullOrEmpty(subcategory.title))
					{
						if (verbose)
						{
							Debug.LogWarning("Database: Subcategory '" + mergeCat.name + "/" + subcategory.name + "' replacing title with '" + subcategory.title + "'");
						}
						category.title = subcategory.title;
					}
				}
				else
				{
					if (verbose)
					{
						Debug.LogWarning("Database: Category '" + mergeCat.name + "' adding '" + subcategory.name + "' subcategory");
					}
					mergeCat.subcategories.Add(subcategory);
				}
			}
		}

		public Category GetCategoryNext(Category category)
		{
			Category category2 = category;
			if (category2.subcategories.Count > 0)
			{
				return category2.subcategories[0];
			}
			if (!IsRootCategory(category2))
			{
				Category rootCategory = GetRootCategory(category2);
				int num = rootCategory.subcategories.IndexOf(category2);
				if (num < rootCategory.subcategories.Count - 1)
				{
					return rootCategory.subcategories[num + 1];
				}
				category2 = rootCategory;
			}
			int num2 = categories.IndexOf(category2);
			if (num2 < categories.Count - 1)
			{
				return categories[num2 + 1];
			}
			return null;
		}

		public Screen GetScreenFirst(Category cat)
		{
			if (!string.IsNullOrEmpty(cat.titleScreen))
			{
				return GetScreen(cat.titleScreen);
			}
			if (cat.screens.Count > 0)
			{
				return GetScreen(cat.screens[0]);
			}
			if (cat.subcategories.Count > 0)
			{
				Screen screen = null;
				for (int i = 0; i < cat.subcategories.Count; i++)
				{
					screen = GetScreenFirst(cat.subcategories[i]);
					if (screen != null)
					{
						return screen;
					}
				}
			}
			return null;
		}

		public Screen GetScreenNext(Screen screen)
		{
			string text = screen.name;
			Category category = GetCategory(screen);
			if (category.titleScreen == text)
			{
				if (category.screens.Count > 0)
				{
					return GetScreen(category.screens[0]);
				}
			}
			else
			{
				int num = category.screens.IndexOf(text);
				if (num < category.screens.Count - 1)
				{
					return GetScreen(category.screens[num + 1]);
				}
			}
			while ((category = GetCategoryNext(category)) != null)
			{
				Screen screenFirst = GetScreenFirst(category);
				if (screenFirst != null)
				{
					return screenFirst;
				}
			}
			return null;
		}

		public Screen GetScreenPrevious(Screen screen)
		{
			string item = screen.name;
			Category category = GetCategory(screen);
			int num = category.screens.IndexOf(item);
			if (num > 0)
			{
				return GetScreen(category.screens[num - 1]);
			}
			if (num == 0 && !string.IsNullOrEmpty(category.titleScreen))
			{
				return GetScreen(category.titleScreen);
			}
			if (!IsRootCategory(category))
			{
				Category rootCategory = GetRootCategory(category);
				if (rootCategory.subcategories.IndexOf(category) == 0 && !string.IsNullOrEmpty(rootCategory.titleScreen))
				{
					return GetScreen(rootCategory.titleScreen);
				}
			}
			while ((category = GetCategoryPrevious(category)) != null)
			{
				Screen screenLast = GetScreenLast(category);
				if (screenLast != null)
				{
					return screenLast;
				}
			}
			return null;
		}

		public Screen GetScreenLast(Category cat)
		{
			if (cat.subcategories.Count > 0)
			{
				Screen screen = null;
				for (int num = cat.subcategories.Count - 1; num >= 0; num--)
				{
					screen = GetScreenLast(cat.subcategories[num]);
					if (screen != null)
					{
						return screen;
					}
				}
			}
			if (cat.screens.Count > 0)
			{
				return GetScreen(cat.screens[cat.screens.Count - 1]);
			}
			if (!string.IsNullOrEmpty(cat.titleScreen))
			{
				return GetScreen(cat.titleScreen);
			}
			return null;
		}

		public Category GetCategoryPrevious(Category category)
		{
			if (!IsRootCategory(category))
			{
				Category rootCategory = GetRootCategory(category);
				int num = rootCategory.subcategories.IndexOf(category);
				if (num > 0)
				{
					return rootCategory.subcategories[num - 1];
				}
				return rootCategory;
			}
			int num2 = categories.IndexOf(category);
			if (num2 > 0)
			{
				Category category2 = categories[num2 - 1];
				if (category2.subcategories.Count > 0)
				{
					return category2.subcategories[category2.subcategories.Count - 1];
				}
				return category2;
			}
			return null;
		}
	}
}

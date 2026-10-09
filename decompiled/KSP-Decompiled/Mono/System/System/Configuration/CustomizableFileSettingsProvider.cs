using System.Collections.Specialized;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Xml;

namespace System.Configuration;

internal class CustomizableFileSettingsProvider : SettingsProvider, IApplicationSettingsProvider
{
	private static Type webConfigurationFileMapType;

	private static string userRoamingPath = "";

	private static string userLocalPath = "";

	private static string userRoamingPathPrevVersion = "";

	private static string userLocalPathPrevVersion = "";

	private static string userRoamingName = "user.config";

	private static string userLocalName = "user.config";

	private static string userRoamingBasePath = "";

	private static string userLocalBasePath = "";

	private static string CompanyName = "";

	private static string ProductName = "";

	private static string ForceVersion = "";

	private static string[] ProductVersion;

	private static bool isVersionMajor = false;

	private static bool isVersionMinor = false;

	private static bool isVersionBuild = false;

	private static bool isVersionRevision = false;

	private static bool isCompany = true;

	private static bool isProduct = true;

	private static bool isEvidence = false;

	private static bool userDefine = false;

	private static UserConfigLocationOption userConfig = UserConfigLocationOption.Company_Product;

	private string app_name = string.Empty;

	private ExeConfigurationFileMap exeMapCurrent;

	private ExeConfigurationFileMap exeMapPrev;

	private SettingsPropertyValueCollection values;

	internal static string UserRoamingFullPath => Path.Combine(userRoamingPath, userRoamingName);

	internal static string UserLocalFullPath => Path.Combine(userLocalPath, userLocalName);

	public static string PrevUserRoamingFullPath => Path.Combine(userRoamingPathPrevVersion, userRoamingName);

	public static string PrevUserLocalFullPath => Path.Combine(userLocalPathPrevVersion, userLocalName);

	public static string UserRoamingPath => userRoamingPath;

	public static string UserLocalPath => userLocalPath;

	public static string UserRoamingName => userRoamingName;

	public static string UserLocalName => userLocalName;

	public static UserConfigLocationOption UserConfigSelector
	{
		get
		{
			return userConfig;
		}
		set
		{
			userConfig = value;
			if ((userConfig & UserConfigLocationOption.Other) != 0)
			{
				isVersionMajor = false;
				isVersionMinor = false;
				isVersionBuild = false;
				isVersionRevision = false;
				isCompany = false;
			}
			else
			{
				isVersionRevision = (userConfig & (UserConfigLocationOption)8u) != 0;
				isVersionBuild = isVersionRevision | ((userConfig & (UserConfigLocationOption)4u) != 0);
				isVersionMinor = isVersionBuild | ((userConfig & (UserConfigLocationOption)2u) != 0);
				isVersionMajor = IsVersionMinor | ((userConfig & (UserConfigLocationOption)1u) != 0);
				isCompany = (userConfig & (UserConfigLocationOption)16u) != 0;
				isProduct = (userConfig & UserConfigLocationOption.Product) != 0;
			}
		}
	}

	public static bool IsVersionMajor
	{
		get
		{
			return isVersionMajor;
		}
		set
		{
			isVersionMajor = value;
			isVersionMinor = false;
			isVersionBuild = false;
			isVersionRevision = false;
		}
	}

	public static bool IsVersionMinor
	{
		get
		{
			return isVersionMinor;
		}
		set
		{
			isVersionMinor = value;
			if (isVersionMinor)
			{
				isVersionMajor = true;
			}
			isVersionBuild = false;
			isVersionRevision = false;
		}
	}

	public static bool IsVersionBuild
	{
		get
		{
			return isVersionBuild;
		}
		set
		{
			isVersionBuild = value;
			if (isVersionBuild)
			{
				isVersionMajor = true;
				isVersionMinor = true;
			}
			isVersionRevision = false;
		}
	}

	public static bool IsVersionRevision
	{
		get
		{
			return isVersionRevision;
		}
		set
		{
			isVersionRevision = value;
			if (isVersionRevision)
			{
				isVersionMajor = true;
				isVersionMinor = true;
				isVersionBuild = true;
			}
		}
	}

	public static bool IsCompany
	{
		get
		{
			return isCompany;
		}
		set
		{
			isCompany = value;
		}
	}

	public static bool IsEvidence
	{
		get
		{
			return isEvidence;
		}
		set
		{
			isEvidence = value;
		}
	}

	public override string Name => base.Name;

	public override string ApplicationName
	{
		get
		{
			return app_name;
		}
		set
		{
			app_name = value;
		}
	}

	public override void Initialize(string name, NameValueCollection config)
	{
		base.Initialize(name, config);
	}

	private static string GetCompanyName()
	{
		Assembly assembly = Assembly.GetEntryAssembly();
		if (assembly == null)
		{
			assembly = Assembly.GetCallingAssembly();
		}
		AssemblyCompanyAttribute[] array = (AssemblyCompanyAttribute[])assembly.GetCustomAttributes(typeof(AssemblyCompanyAttribute), inherit: true);
		if (array != null && array.Length != 0)
		{
			return array[0].Company;
		}
		MethodInfo entryPoint = assembly.EntryPoint;
		Type type = ((entryPoint != null) ? entryPoint.DeclaringType : null);
		if (type != null && !string.IsNullOrEmpty(type.Namespace))
		{
			int num = type.Namespace.IndexOf('.');
			if (num >= 0)
			{
				return type.Namespace.Substring(0, num);
			}
			return type.Namespace;
		}
		return "Program";
	}

	private static string GetProductName()
	{
		Assembly assembly = Assembly.GetEntryAssembly();
		if (assembly == null)
		{
			assembly = Assembly.GetCallingAssembly();
		}
		byte[] publicKeyToken = assembly.GetName().GetPublicKeyToken();
		return string.Format("{0}_{1}_{2}", AppDomain.CurrentDomain.FriendlyName, (publicKeyToken != null && publicKeyToken.Length != 0) ? "StrongName" : "Url", GetEvidenceHash());
	}

	private static string GetEvidenceHash()
	{
		Assembly assembly = Assembly.GetEntryAssembly();
		if (assembly == null)
		{
			assembly = Assembly.GetCallingAssembly();
		}
		byte[] publicKeyToken = assembly.GetName().GetPublicKeyToken();
		byte[] array = SHA1.Create().ComputeHash((publicKeyToken != null && publicKeyToken.Length != 0) ? publicKeyToken : Encoding.UTF8.GetBytes(assembly.EscapedCodeBase));
		StringBuilder stringBuilder = new StringBuilder();
		byte[] array2 = array;
		foreach (byte b in array2)
		{
			stringBuilder.AppendFormat("{0:x2}", b);
		}
		return stringBuilder.ToString();
	}

	private static string GetProductVersion()
	{
		Assembly assembly = Assembly.GetEntryAssembly();
		if (assembly == null)
		{
			assembly = Assembly.GetCallingAssembly();
		}
		if (assembly == null)
		{
			return string.Empty;
		}
		return assembly.GetName().Version.ToString();
	}

	private static void CreateUserConfigPath()
	{
		if (userDefine)
		{
			return;
		}
		if (ProductName == "")
		{
			ProductName = GetProductName();
		}
		if (CompanyName == "")
		{
			CompanyName = GetCompanyName();
		}
		if (ForceVersion == "")
		{
			ProductVersion = GetProductVersion().Split('.');
		}
		if (userRoamingBasePath == "")
		{
			userRoamingPath = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
		}
		else
		{
			userRoamingPath = userRoamingBasePath;
		}
		if (userLocalBasePath == "")
		{
			userLocalPath = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
		}
		else
		{
			userLocalPath = userLocalBasePath;
		}
		if (isCompany)
		{
			userRoamingPath = Path.Combine(userRoamingPath, CompanyName);
			userLocalPath = Path.Combine(userLocalPath, CompanyName);
		}
		if (isProduct)
		{
			if (isEvidence)
			{
				Assembly assembly = Assembly.GetEntryAssembly();
				if (assembly == null)
				{
					assembly = Assembly.GetCallingAssembly();
				}
				byte[] publicKeyToken = assembly.GetName().GetPublicKeyToken();
				ProductName = string.Format("{0}_{1}_{2}", ProductName, (publicKeyToken != null) ? "StrongName" : "Url", GetEvidenceHash());
			}
			userRoamingPath = Path.Combine(userRoamingPath, ProductName);
			userLocalPath = Path.Combine(userLocalPath, ProductName);
		}
		string text = ((!(ForceVersion == "")) ? ForceVersion : (isVersionRevision ? $"{ProductVersion[0]}.{ProductVersion[1]}.{ProductVersion[2]}.{ProductVersion[3]}" : (isVersionBuild ? $"{ProductVersion[0]}.{ProductVersion[1]}.{ProductVersion[2]}" : (isVersionMinor ? $"{ProductVersion[0]}.{ProductVersion[1]}" : ((!isVersionMajor) ? "" : ProductVersion[0])))));
		string text2 = PrevVersionPath(userRoamingPath, text);
		string text3 = PrevVersionPath(userLocalPath, text);
		userRoamingPath = Path.Combine(userRoamingPath, text);
		userLocalPath = Path.Combine(userLocalPath, text);
		if (text2 != "")
		{
			userRoamingPathPrevVersion = Path.Combine(userRoamingPath, text2);
		}
		if (text3 != "")
		{
			userLocalPathPrevVersion = Path.Combine(userLocalPath, text3);
		}
	}

	private static string PrevVersionPath(string dirName, string currentVersion)
	{
		string text = "";
		if (!Directory.Exists(dirName))
		{
			return text;
		}
		DirectoryInfo[] directories = new DirectoryInfo(dirName).GetDirectories();
		foreach (DirectoryInfo directoryInfo in directories)
		{
			if (string.Compare(currentVersion, directoryInfo.Name, StringComparison.Ordinal) > 0 && string.Compare(text, directoryInfo.Name, StringComparison.Ordinal) < 0)
			{
				text = directoryInfo.Name;
			}
		}
		return text;
	}

	public static bool SetUserRoamingPath(string configPath)
	{
		if (CheckPath(configPath))
		{
			userRoamingBasePath = configPath;
			return true;
		}
		return false;
	}

	public static bool SetUserLocalPath(string configPath)
	{
		if (CheckPath(configPath))
		{
			userLocalBasePath = configPath;
			return true;
		}
		return false;
	}

	private static bool CheckFileName(string configFile)
	{
		return configFile.IndexOfAny(Path.GetInvalidFileNameChars()) < 0;
	}

	public static bool SetUserRoamingFileName(string configFile)
	{
		if (CheckFileName(configFile))
		{
			userRoamingName = configFile;
			return true;
		}
		return false;
	}

	public static bool SetUserLocalFileName(string configFile)
	{
		if (CheckFileName(configFile))
		{
			userLocalName = configFile;
			return true;
		}
		return false;
	}

	public static bool SetCompanyName(string companyName)
	{
		if (CheckFileName(companyName))
		{
			CompanyName = companyName;
			return true;
		}
		return false;
	}

	public static bool SetProductName(string productName)
	{
		if (CheckFileName(productName))
		{
			ProductName = productName;
			return true;
		}
		return false;
	}

	public static bool SetVersion(int major)
	{
		ForceVersion = $"{major}";
		return true;
	}

	public static bool SetVersion(int major, int minor)
	{
		ForceVersion = $"{major}.{minor}";
		return true;
	}

	public static bool SetVersion(int major, int minor, int build)
	{
		ForceVersion = $"{major}.{minor}.{build}";
		return true;
	}

	public static bool SetVersion(int major, int minor, int build, int revision)
	{
		ForceVersion = $"{major}.{minor}.{build}.{revision}";
		return true;
	}

	public static bool SetVersion(string forceVersion)
	{
		if (CheckFileName(forceVersion))
		{
			ForceVersion = forceVersion;
			return true;
		}
		return false;
	}

	private static bool CheckPath(string configPath)
	{
		char[] invalidPathChars = Path.GetInvalidPathChars();
		if (configPath.IndexOfAny(invalidPathChars) >= 0)
		{
			return false;
		}
		string path = configPath;
		string fileName;
		while ((fileName = Path.GetFileName(path)) != "")
		{
			if (!CheckFileName(fileName))
			{
				return false;
			}
			path = Path.GetDirectoryName(path);
		}
		return true;
	}

	private string StripXmlHeader(string serializedValue)
	{
		if (serializedValue == null)
		{
			return string.Empty;
		}
		XmlElement xmlElement = new XmlDocument().CreateElement("value");
		xmlElement.InnerXml = serializedValue;
		foreach (XmlNode childNode in xmlElement.ChildNodes)
		{
			if (childNode.NodeType == XmlNodeType.XmlDeclaration)
			{
				xmlElement.RemoveChild(childNode);
				break;
			}
		}
		return xmlElement.InnerXml;
	}

	private void SaveProperties(ExeConfigurationFileMap exeMap, SettingsPropertyValueCollection collection, ConfigurationUserLevel level, SettingsContext context, bool checkUserLevel)
	{
		Configuration configuration = ConfigurationManager.OpenMappedExeConfiguration(exeMap, level);
		UserSettingsGroup userSettingsGroup = configuration.GetSectionGroup("userSettings") as UserSettingsGroup;
		bool flag = level == ConfigurationUserLevel.PerUserRoaming;
		if (userSettingsGroup == null)
		{
			userSettingsGroup = new UserSettingsGroup();
			configuration.SectionGroups.Add("userSettings", userSettingsGroup);
		}
		ApplicationSettingsBase currentSettings = context.CurrentSettings;
		string name = NormalizeInvalidXmlChars(((currentSettings != null) ? currentSettings.GetType() : typeof(ApplicationSettingsBase)).FullName);
		ClientSettingsSection clientSettingsSection = null;
		clientSettingsSection = userSettingsGroup.Sections.Get(name) as ClientSettingsSection;
		if (clientSettingsSection == null)
		{
			clientSettingsSection = new ClientSettingsSection();
			userSettingsGroup.Sections.Add(name, clientSettingsSection);
		}
		bool flag2 = false;
		if (clientSettingsSection == null)
		{
			return;
		}
		foreach (SettingsPropertyValue item in collection)
		{
			if ((!checkUserLevel || item.Property.Attributes.Contains(typeof(SettingsManageabilityAttribute)) == flag) && !item.Property.Attributes.Contains(typeof(ApplicationScopedSettingAttribute)))
			{
				flag2 = true;
				SettingElement settingElement = clientSettingsSection.Settings.Get(item.Name);
				if (settingElement == null)
				{
					settingElement = new SettingElement(item.Name, item.Property.SerializeAs);
					clientSettingsSection.Settings.Add(settingElement);
				}
				if (settingElement.Value.ValueXml == null)
				{
					settingElement.Value.ValueXml = new XmlDocument().CreateElement("value");
				}
				switch (item.Property.SerializeAs)
				{
				case SettingsSerializeAs.Xml:
					settingElement.Value.ValueXml.InnerXml = StripXmlHeader(item.SerializedValue as string);
					break;
				case SettingsSerializeAs.String:
					settingElement.Value.ValueXml.InnerText = item.SerializedValue as string;
					break;
				case SettingsSerializeAs.Binary:
					settingElement.Value.ValueXml.InnerText = ((item.SerializedValue != null) ? Convert.ToBase64String(item.SerializedValue as byte[]) : string.Empty);
					break;
				default:
					throw new NotImplementedException();
				}
			}
		}
		if (flag2)
		{
			configuration.Save(ConfigurationSaveMode.Minimal, forceSaveAll: true);
		}
	}

	private string NormalizeInvalidXmlChars(string str)
	{
		char[] anyOf = new char[1] { '+' };
		if (str == null || str.IndexOfAny(anyOf) == -1)
		{
			return str;
		}
		str = str.Replace("+", "_x002B_");
		return str;
	}

	private void LoadPropertyValue(SettingsPropertyCollection collection, SettingElement element, bool allowOverwrite)
	{
		SettingsProperty settingsProperty = collection[element.Name];
		if (settingsProperty == null)
		{
			settingsProperty = new SettingsProperty(element.Name);
			collection.Add(settingsProperty);
		}
		SettingsPropertyValue settingsPropertyValue = new SettingsPropertyValue(settingsProperty);
		settingsPropertyValue.IsDirty = false;
		if (element.Value.ValueXml != null)
		{
			switch (settingsPropertyValue.Property.SerializeAs)
			{
			case SettingsSerializeAs.Xml:
				settingsPropertyValue.SerializedValue = element.Value.ValueXml.InnerXml;
				break;
			case SettingsSerializeAs.String:
				settingsPropertyValue.SerializedValue = element.Value.ValueXml.InnerText.Trim();
				break;
			case SettingsSerializeAs.Binary:
				settingsPropertyValue.SerializedValue = Convert.FromBase64String(element.Value.ValueXml.InnerText);
				break;
			}
		}
		else
		{
			settingsPropertyValue.SerializedValue = settingsProperty.DefaultValue;
		}
		try
		{
			if (allowOverwrite)
			{
				values.Remove(element.Name);
			}
			values.Add(settingsPropertyValue);
		}
		catch (ArgumentException inner)
		{
			throw new ConfigurationErrorsException(string.Format(CultureInfo.InvariantCulture, "Failed to load value for '{0}'.", element.Name), inner);
		}
	}

	private void LoadProperties(ExeConfigurationFileMap exeMap, SettingsPropertyCollection collection, ConfigurationUserLevel level, string sectionGroupName, bool allowOverwrite, string groupName)
	{
		ConfigurationSectionGroup sectionGroup = ConfigurationManager.OpenMappedExeConfiguration(exeMap, level).GetSectionGroup(sectionGroupName);
		if (sectionGroup == null)
		{
			return;
		}
		foreach (ConfigurationSection section in sectionGroup.Sections)
		{
			if (section.SectionInformation.Name != groupName || !(section is ClientSettingsSection clientSettingsSection))
			{
				continue;
			}
			{
				foreach (SettingElement setting in clientSettingsSection.Settings)
				{
					LoadPropertyValue(collection, setting, allowOverwrite);
				}
				break;
			}
		}
	}

	public override void SetPropertyValues(SettingsContext context, SettingsPropertyValueCollection collection)
	{
		CreateExeMap();
		if (UserLocalFullPath == UserRoamingFullPath)
		{
			SaveProperties(exeMapCurrent, collection, ConfigurationUserLevel.PerUserRoaming, context, checkUserLevel: false);
			return;
		}
		SaveProperties(exeMapCurrent, collection, ConfigurationUserLevel.PerUserRoaming, context, checkUserLevel: true);
		SaveProperties(exeMapCurrent, collection, ConfigurationUserLevel.PerUserRoamingAndLocal, context, checkUserLevel: true);
	}

	public override SettingsPropertyValueCollection GetPropertyValues(SettingsContext context, SettingsPropertyCollection collection)
	{
		CreateExeMap();
		values = new SettingsPropertyValueCollection();
		string str = context["GroupName"] as string;
		str = NormalizeInvalidXmlChars(str);
		LoadProperties(exeMapCurrent, collection, ConfigurationUserLevel.None, "applicationSettings", allowOverwrite: false, str);
		LoadProperties(exeMapCurrent, collection, ConfigurationUserLevel.None, "userSettings", allowOverwrite: false, str);
		LoadProperties(exeMapCurrent, collection, ConfigurationUserLevel.PerUserRoaming, "userSettings", allowOverwrite: true, str);
		LoadProperties(exeMapCurrent, collection, ConfigurationUserLevel.PerUserRoamingAndLocal, "userSettings", allowOverwrite: true, str);
		foreach (SettingsProperty item in collection)
		{
			if (values[item.Name] == null)
			{
				values.Add(new SettingsPropertyValue(item));
			}
		}
		return values;
	}

	private void CreateExeMap()
	{
		if (exeMapCurrent != null)
		{
			return;
		}
		CreateUserConfigPath();
		exeMapCurrent = new ExeConfigurationFileMap();
		Assembly assembly = Assembly.GetEntryAssembly() ?? Assembly.GetExecutingAssembly();
		exeMapCurrent.ExeConfigFilename = assembly.Location + ".config";
		exeMapCurrent.LocalUserConfigFilename = UserLocalFullPath;
		exeMapCurrent.RoamingUserConfigFilename = UserRoamingFullPath;
		if (webConfigurationFileMapType != null && typeof(ConfigurationFileMap).IsAssignableFrom(webConfigurationFileMapType))
		{
			try
			{
				if (Activator.CreateInstance(webConfigurationFileMapType) is ConfigurationFileMap { MachineConfigFilename: var machineConfigFilename } && !string.IsNullOrEmpty(machineConfigFilename))
				{
					exeMapCurrent.ExeConfigFilename = machineConfigFilename;
				}
			}
			catch
			{
			}
		}
		if (PrevUserLocalFullPath != "" && PrevUserRoamingFullPath != "")
		{
			exeMapPrev = new ExeConfigurationFileMap();
			exeMapPrev.ExeConfigFilename = assembly.Location + ".config";
			exeMapPrev.LocalUserConfigFilename = PrevUserLocalFullPath;
			exeMapPrev.RoamingUserConfigFilename = PrevUserRoamingFullPath;
		}
	}

	public SettingsPropertyValue GetPreviousVersion(SettingsContext context, SettingsProperty property)
	{
		return null;
	}

	public void Reset(SettingsContext context)
	{
		if (values == null)
		{
			return;
		}
		foreach (SettingsPropertyValue value in values)
		{
			values[value.Name].PropertyValue = value.Reset();
		}
	}

	public void Upgrade(SettingsContext context, SettingsPropertyCollection properties)
	{
	}

	public static void setCreate()
	{
		CreateUserConfigPath();
	}
}

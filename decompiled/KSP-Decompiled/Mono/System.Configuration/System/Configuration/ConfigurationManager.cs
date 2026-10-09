using System.Collections.Specialized;
using System.Configuration.Internal;
using System.IO;
using System.Reflection;
using System.Text;
using Unity;

namespace System.Configuration;

public static class ConfigurationManager
{
	private static InternalConfigurationFactory configFactory = new InternalConfigurationFactory();

	private static IInternalConfigSystem configSystem = new ClientConfigurationSystem();

	private static object lockobj = new object();

	internal static IInternalConfigConfigurationFactory ConfigurationFactory => configFactory;

	internal static IInternalConfigSystem ConfigurationSystem => configSystem;

	public static NameValueCollection AppSettings => (NameValueCollection)GetSection("appSettings");

	public static ConnectionStringSettingsCollection ConnectionStrings => ((ConnectionStringsSection)GetSection("connectionStrings")).ConnectionStrings;

	[System.MonoTODO("Evidence and version still needs work")]
	private static string GetAssemblyInfo(Assembly a)
	{
		object[] customAttributes = a.GetCustomAttributes(typeof(AssemblyProductAttribute), inherit: false);
		string arg = ((customAttributes == null || customAttributes.Length == 0) ? AppDomain.CurrentDomain.FriendlyName : ((AssemblyProductAttribute)customAttributes[0]).Product);
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append("evidencehere");
		string arg2 = stringBuilder.ToString();
		customAttributes = a.GetCustomAttributes(typeof(AssemblyVersionAttribute), inherit: false);
		return Path.Combine(path2: (customAttributes == null || customAttributes.Length == 0) ? "1.0.0.0" : ((AssemblyVersionAttribute)customAttributes[0]).Version, path1: $"{arg}_{arg2}");
	}

	internal static Configuration OpenExeConfigurationInternal(ConfigurationUserLevel userLevel, Assembly calling_assembly, string exePath)
	{
		ExeConfigurationFileMap exeConfigurationFileMap = new ExeConfigurationFileMap();
		if (userLevel != 0)
		{
			if (userLevel != ConfigurationUserLevel.PerUserRoaming)
			{
				if (userLevel != ConfigurationUserLevel.PerUserRoamingAndLocal)
				{
					goto IL_00ea;
				}
				exeConfigurationFileMap.LocalUserConfigFilename = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), GetAssemblyInfo(calling_assembly));
				exeConfigurationFileMap.LocalUserConfigFilename = Path.Combine(exeConfigurationFileMap.LocalUserConfigFilename, "user.config");
			}
			exeConfigurationFileMap.RoamingUserConfigFilename = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), GetAssemblyInfo(calling_assembly));
			exeConfigurationFileMap.RoamingUserConfigFilename = Path.Combine(exeConfigurationFileMap.RoamingUserConfigFilename, "user.config");
		}
		if (exePath == null || exePath.Length == 0)
		{
			exeConfigurationFileMap.ExeConfigFilename = AppDomain.CurrentDomain.SetupInformation.ConfigurationFile;
		}
		else
		{
			if (!Path.IsPathRooted(exePath))
			{
				exePath = Path.GetFullPath(exePath);
			}
			if (!File.Exists(exePath))
			{
				Exception inner = new ArgumentException("The specified path does not exist.", "exePath");
				throw new ConfigurationErrorsException("Error Initializing the configuration system:", inner);
			}
			exeConfigurationFileMap.ExeConfigFilename = exePath + ".config";
		}
		goto IL_00ea;
		IL_00ea:
		return ConfigurationFactory.Create(typeof(ExeConfigurationHost), exeConfigurationFileMap, userLevel);
	}

	public static Configuration OpenExeConfiguration(ConfigurationUserLevel userLevel)
	{
		return OpenExeConfigurationInternal(userLevel, Assembly.GetEntryAssembly() ?? Assembly.GetCallingAssembly(), null);
	}

	public static Configuration OpenExeConfiguration(string exePath)
	{
		return OpenExeConfigurationInternal(ConfigurationUserLevel.None, Assembly.GetEntryAssembly() ?? Assembly.GetCallingAssembly(), exePath);
	}

	[System.MonoLimitation("ConfigurationUserLevel parameter is not supported.")]
	public static Configuration OpenMappedExeConfiguration(ExeConfigurationFileMap fileMap, ConfigurationUserLevel userLevel)
	{
		return ConfigurationFactory.Create(typeof(ExeConfigurationHost), fileMap, userLevel);
	}

	public static Configuration OpenMachineConfiguration()
	{
		ConfigurationFileMap configurationFileMap = new ConfigurationFileMap();
		return ConfigurationFactory.Create(typeof(MachineConfigurationHost), configurationFileMap);
	}

	public static Configuration OpenMappedMachineConfiguration(ConfigurationFileMap fileMap)
	{
		return ConfigurationFactory.Create(typeof(MachineConfigurationHost), fileMap);
	}

	public static object GetSection(string sectionName)
	{
		object section = ConfigurationSystem.GetSection(sectionName);
		if (section is ConfigurationSection)
		{
			return ((ConfigurationSection)section).GetRuntimeObject();
		}
		return section;
	}

	public static void RefreshSection(string sectionName)
	{
		ConfigurationSystem.RefreshConfig(sectionName);
	}

	internal static IInternalConfigSystem ChangeConfigurationSystem(IInternalConfigSystem newSystem)
	{
		if (newSystem == null)
		{
			throw new ArgumentNullException("newSystem");
		}
		lock (lockobj)
		{
			IInternalConfigSystem result = configSystem;
			configSystem = newSystem;
			return result;
		}
	}

	public static Configuration OpenMappedExeConfiguration(ExeConfigurationFileMap fileMap, ConfigurationUserLevel userLevel, bool preLoad)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

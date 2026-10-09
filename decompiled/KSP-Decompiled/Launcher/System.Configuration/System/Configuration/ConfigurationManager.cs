using System.Collections.Specialized;
using Unity;

namespace System.Configuration;

public static class ConfigurationManager
{
	public static NameValueCollection AppSettings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public static ConnectionStringSettingsCollection ConnectionStrings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public static object GetSection(string sectionName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Configuration OpenExeConfiguration(ConfigurationUserLevel userLevel)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Configuration OpenExeConfiguration(string exePath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Configuration OpenMachineConfiguration()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Configuration OpenMappedExeConfiguration(ExeConfigurationFileMap fileMap, ConfigurationUserLevel userLevel)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Configuration OpenMappedExeConfiguration(ExeConfigurationFileMap fileMap, ConfigurationUserLevel userLevel, bool preLoad)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Configuration OpenMappedMachineConfiguration(ConfigurationFileMap fileMap)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static void RefreshSection(string sectionName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

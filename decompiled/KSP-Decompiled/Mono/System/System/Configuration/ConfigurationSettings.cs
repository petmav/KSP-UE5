using System.Collections.Specialized;

namespace System.Configuration;

public sealed class ConfigurationSettings
{
	private static IConfigurationSystem config = DefaultConfig.GetInstance();

	private static object lockobj = new object();

	[Obsolete("This property is obsolete.  Please use System.Configuration.ConfigurationManager.AppSettings")]
	public static NameValueCollection AppSettings
	{
		get
		{
			object obj = ConfigurationManager.GetSection("appSettings");
			if (obj == null)
			{
				obj = new NameValueCollection();
			}
			return (NameValueCollection)obj;
		}
	}

	private ConfigurationSettings()
	{
	}

	[Obsolete("This method is obsolete, it has been replaced by System.Configuration!System.Configuration.ConfigurationManager.GetSection")]
	public static object GetConfig(string sectionName)
	{
		return ConfigurationManager.GetSection(sectionName);
	}

	internal static IConfigurationSystem ChangeConfigurationSystem(IConfigurationSystem newSystem)
	{
		if (newSystem == null)
		{
			throw new ArgumentNullException("newSystem");
		}
		lock (lockobj)
		{
			IConfigurationSystem result = config;
			config = newSystem;
			return result;
		}
	}
}

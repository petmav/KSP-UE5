using System.Collections.Specialized;
using System.Configuration;

namespace System.Xml.Serialization;

internal static class AppSettings
{
	private const string UseLegacySerializerGenerationAppSettingsString = "System:Xml:Serialization:UseLegacySerializerGeneration";

	private static bool? useLegacySerializerGeneration;

	private static volatile bool settingsInitalized = false;

	private static object appSettingsLock = new object();

	internal static bool? UseLegacySerializerGeneration
	{
		get
		{
			EnsureSettingsLoaded();
			return useLegacySerializerGeneration;
		}
	}

	private static void EnsureSettingsLoaded()
	{
		if (settingsInitalized)
		{
			return;
		}
		lock (appSettingsLock)
		{
			if (settingsInitalized)
			{
				return;
			}
			NameValueCollection nameValueCollection = null;
			try
			{
				nameValueCollection = ConfigurationManager.AppSettings;
			}
			catch (ConfigurationErrorsException)
			{
			}
			finally
			{
				if (nameValueCollection == null || !bool.TryParse(nameValueCollection["System:Xml:Serialization:UseLegacySerializerGeneration"], out var result))
				{
					useLegacySerializerGeneration = null;
				}
				else
				{
					useLegacySerializerGeneration = result;
				}
				settingsInitalized = true;
			}
		}
	}
}

namespace System.Configuration;

public static class ProtectedConfiguration
{
	public const string DataProtectionProviderName = "DataProtectionConfigurationProvider";

	public const string ProtectedDataSectionName = "configProtectedData";

	public const string RsaProviderName = "RsaProtectedConfigurationProvider";

	public static string DefaultProvider => Section.DefaultProvider;

	public static ProtectedConfigurationProviderCollection Providers => Section.GetAllProviders();

	internal static ProtectedConfigurationSection Section => (ProtectedConfigurationSection)ConfigurationManager.GetSection("configProtectedData");

	internal static ProtectedConfigurationProvider GetProvider(string name, bool throwOnError)
	{
		ProtectedConfigurationProvider protectedConfigurationProvider = Providers[name];
		if (protectedConfigurationProvider == null && throwOnError)
		{
			throw new Exception($"The protection provider '{name}' was not found.");
		}
		return protectedConfigurationProvider;
	}
}

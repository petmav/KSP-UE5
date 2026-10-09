using System.Security.Permissions;
using Unity;

namespace System.Configuration;

[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public static class ProtectedConfiguration
{
	public const string DataProtectionProviderName = "DataProtectionConfigurationProvider";

	public const string ProtectedDataSectionName = "configProtectedData";

	public const string RsaProviderName = "RsaProtectedConfigurationProvider";

	public static string DefaultProvider
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public static ProtectedConfigurationProviderCollection Providers
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}
}

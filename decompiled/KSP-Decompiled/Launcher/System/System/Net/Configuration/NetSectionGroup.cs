using System.Configuration;
using Unity;

namespace System.Net.Configuration;

public sealed class NetSectionGroup : ConfigurationSectionGroup
{
	public AuthenticationModulesSection AuthenticationModules
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ConnectionManagementSection ConnectionManagement
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public DefaultProxySection DefaultProxy
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public MailSettingsSectionGroup MailSettings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public RequestCachingSection RequestCaching
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SettingsSection Settings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public WebRequestModulesSection WebRequestModules
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public NetSectionGroup()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static NetSectionGroup GetSectionGroup(System.Configuration.Configuration config)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

using System.Collections.Specialized;
using System.Security.Permissions;

namespace System.Configuration;

public class LocalFileSettingsProvider : SettingsProvider, IApplicationSettingsProvider
{
	private CustomizableFileSettingsProvider impl;

	public override string ApplicationName
	{
		get
		{
			return impl.ApplicationName;
		}
		set
		{
			impl.ApplicationName = value;
		}
	}

	public LocalFileSettingsProvider()
	{
		impl = new CustomizableFileSettingsProvider();
	}

	[System.MonoTODO]
	[FileIOPermission(SecurityAction.Assert, AllFiles = (FileIOPermissionAccess.Read | FileIOPermissionAccess.PathDiscovery))]
	[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
	[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
	public SettingsPropertyValue GetPreviousVersion(SettingsContext context, SettingsProperty property)
	{
		return impl.GetPreviousVersion(context, property);
	}

	[System.MonoTODO]
	public override SettingsPropertyValueCollection GetPropertyValues(SettingsContext context, SettingsPropertyCollection properties)
	{
		return impl.GetPropertyValues(context, properties);
	}

	public override void Initialize(string name, NameValueCollection values)
	{
		if (name == null)
		{
			name = "LocalFileSettingsProvider";
		}
		if (values != null)
		{
			impl.ApplicationName = values["applicationName"];
		}
		base.Initialize(name, values);
	}

	[System.MonoTODO]
	public void Reset(SettingsContext context)
	{
		impl.Reset(context);
	}

	[System.MonoTODO]
	public override void SetPropertyValues(SettingsContext context, SettingsPropertyValueCollection values)
	{
		impl.SetPropertyValues(context, values);
	}

	[System.MonoTODO]
	public void Upgrade(SettingsContext context, SettingsPropertyCollection properties)
	{
		impl.Upgrade(context, properties);
	}
}

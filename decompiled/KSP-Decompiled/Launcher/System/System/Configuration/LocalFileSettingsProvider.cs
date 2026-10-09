using System.Collections.Specialized;
using System.Security.Permissions;
using Unity;

namespace System.Configuration;

[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public class LocalFileSettingsProvider : SettingsProvider, IApplicationSettingsProvider
{
	public override string ApplicationName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public LocalFileSettingsProvider()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
	[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
	[FileIOPermission(SecurityAction.Assert, AllFiles = (FileIOPermissionAccess.Read | FileIOPermissionAccess.PathDiscovery))]
	public SettingsPropertyValue GetPreviousVersion(SettingsContext context, SettingsProperty property)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override SettingsPropertyValueCollection GetPropertyValues(SettingsContext context, SettingsPropertyCollection properties)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override void Initialize(string name, NameValueCollection values)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Reset(SettingsContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override void SetPropertyValues(SettingsContext context, SettingsPropertyValueCollection values)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Upgrade(SettingsContext context, SettingsPropertyCollection properties)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

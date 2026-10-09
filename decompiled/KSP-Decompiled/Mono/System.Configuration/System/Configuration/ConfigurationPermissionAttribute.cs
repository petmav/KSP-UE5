using System.Security;
using System.Security.Permissions;

namespace System.Configuration;

[Serializable]
[AttributeUsage(AttributeTargets.All, AllowMultiple = true, Inherited = false)]
public sealed class ConfigurationPermissionAttribute : CodeAccessSecurityAttribute
{
	public ConfigurationPermissionAttribute(SecurityAction action)
		: base(action)
	{
	}

	public override IPermission CreatePermission()
	{
		return new ConfigurationPermission(base.Unrestricted ? PermissionState.Unrestricted : PermissionState.None);
	}
}

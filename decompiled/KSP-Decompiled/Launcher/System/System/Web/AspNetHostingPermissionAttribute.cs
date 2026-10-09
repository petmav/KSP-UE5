using System.Security;
using System.Security.Permissions;
using Unity;

namespace System.Web;

[Serializable]
[AttributeUsage(AttributeTargets.All, AllowMultiple = true, Inherited = false)]
public sealed class AspNetHostingPermissionAttribute : CodeAccessSecurityAttribute
{
	public AspNetHostingPermissionLevel Level
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(AspNetHostingPermissionLevel);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public AspNetHostingPermissionAttribute(SecurityAction action)
	{
	}

	public override IPermission CreatePermission()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

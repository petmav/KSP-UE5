using System.Security;
using System.Security.Permissions;
using Unity;

namespace System.Drawing.Printing;

[AttributeUsage(AttributeTargets.All, AllowMultiple = true)]
public sealed class PrintingPermissionAttribute : CodeAccessSecurityAttribute
{
	public PrintingPermissionLevel Level
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(PrintingPermissionLevel);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public PrintingPermissionAttribute(SecurityAction action)
	{
	}

	public override IPermission CreatePermission()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

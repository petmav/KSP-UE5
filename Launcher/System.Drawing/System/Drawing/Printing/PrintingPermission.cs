using System.Security;
using System.Security.Permissions;
using Unity;

namespace System.Drawing.Printing;

[Serializable]
public sealed class PrintingPermission : CodeAccessPermission, IUnrestrictedPermission
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

	public PrintingPermission(PrintingPermissionLevel printingLevel)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public PrintingPermission(PermissionState state)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override IPermission Copy()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override void FromXml(SecurityElement esd)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override IPermission Intersect(IPermission target)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override bool IsSubsetOf(IPermission target)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsUnrestricted()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public override SecurityElement ToXml()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

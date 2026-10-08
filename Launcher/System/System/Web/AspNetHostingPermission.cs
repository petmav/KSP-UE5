using System.Security;
using System.Security.Permissions;
using Unity;

namespace System.Web;

[Serializable]
public sealed class AspNetHostingPermission : CodeAccessPermission, IUnrestrictedPermission
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

	public AspNetHostingPermission(PermissionState state)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public AspNetHostingPermission(AspNetHostingPermissionLevel level)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override IPermission Copy()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override void FromXml(SecurityElement securityElement)
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

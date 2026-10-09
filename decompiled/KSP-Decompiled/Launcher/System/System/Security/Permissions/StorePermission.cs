using Unity;

namespace System.Security.Permissions;

[Serializable]
public sealed class StorePermission : CodeAccessPermission, IUnrestrictedPermission
{
	public StorePermissionFlags Flags
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(StorePermissionFlags);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public StorePermission(PermissionState state)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public StorePermission(StorePermissionFlags flag)
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

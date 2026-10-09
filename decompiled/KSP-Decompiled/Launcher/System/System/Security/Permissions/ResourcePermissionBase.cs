using Unity;

namespace System.Security.Permissions;

[Serializable]
[SecurityPermission(SecurityAction.InheritanceDemand, ControlEvidence = true, ControlPolicy = true)]
public abstract class ResourcePermissionBase : CodeAccessPermission, IUnrestrictedPermission
{
	public const string Any = "*";

	public const string Local = ".";

	protected Type PermissionAccessType
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

	protected string[] TagNames
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

	protected ResourcePermissionBase()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected ResourcePermissionBase(PermissionState state)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected void AddPermissionAccess(ResourcePermissionBaseEntry entry)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected void Clear()
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

	protected ResourcePermissionBaseEntry[] GetPermissionEntries()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
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

	protected void RemovePermissionAccess(ResourcePermissionBaseEntry entry)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override SecurityElement ToXml()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

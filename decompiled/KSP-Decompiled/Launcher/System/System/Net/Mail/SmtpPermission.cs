using System.Security;
using System.Security.Permissions;
using Unity;

namespace System.Net.Mail;

[Serializable]
public sealed class SmtpPermission : CodeAccessPermission, IUnrestrictedPermission
{
	public SmtpAccess Access
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(SmtpAccess);
		}
	}

	public SmtpPermission(bool unrestricted)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SmtpPermission(SmtpAccess access)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SmtpPermission(PermissionState state)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddPermission(SmtpAccess access)
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

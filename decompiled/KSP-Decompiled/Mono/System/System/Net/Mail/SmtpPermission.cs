using System.Security;
using System.Security.Permissions;

namespace System.Net.Mail;

[Serializable]
public sealed class SmtpPermission : CodeAccessPermission, IUnrestrictedPermission
{
	private const int version = 1;

	private bool unrestricted;

	private SmtpAccess access;

	public SmtpAccess Access => access;

	public SmtpPermission(bool unrestricted)
	{
		this.unrestricted = unrestricted;
		access = (unrestricted ? SmtpAccess.ConnectToUnrestrictedPort : SmtpAccess.None);
	}

	public SmtpPermission(PermissionState state)
	{
		unrestricted = state == PermissionState.Unrestricted;
		access = (unrestricted ? SmtpAccess.ConnectToUnrestrictedPort : SmtpAccess.None);
	}

	public SmtpPermission(SmtpAccess access)
	{
		this.access = access;
	}

	public void AddPermission(SmtpAccess access)
	{
		if (!unrestricted && access > this.access)
		{
			this.access = access;
		}
	}

	public override IPermission Copy()
	{
		if (unrestricted)
		{
			return new SmtpPermission(unrestricted: true);
		}
		return new SmtpPermission(access);
	}

	public override IPermission Intersect(IPermission target)
	{
		SmtpPermission smtpPermission = Cast(target);
		if (smtpPermission == null)
		{
			return null;
		}
		if (unrestricted && smtpPermission.unrestricted)
		{
			return new SmtpPermission(unrestricted: true);
		}
		if (access > smtpPermission.access)
		{
			return new SmtpPermission(smtpPermission.access);
		}
		return new SmtpPermission(access);
	}

	public override bool IsSubsetOf(IPermission target)
	{
		SmtpPermission smtpPermission = Cast(target);
		if (smtpPermission == null)
		{
			return IsEmpty();
		}
		if (unrestricted)
		{
			return smtpPermission.unrestricted;
		}
		return access <= smtpPermission.access;
	}

	public bool IsUnrestricted()
	{
		return unrestricted;
	}

	public override SecurityElement ToXml()
	{
		SecurityElement securityElement = PermissionHelper.Element(typeof(SmtpPermission), 1);
		if (unrestricted)
		{
			securityElement.AddAttribute("Unrestricted", "true");
		}
		else
		{
			switch (access)
			{
			case SmtpAccess.ConnectToUnrestrictedPort:
				securityElement.AddAttribute("Access", "ConnectToUnrestrictedPort");
				break;
			case SmtpAccess.Connect:
				securityElement.AddAttribute("Access", "Connect");
				break;
			}
		}
		return securityElement;
	}

	public override void FromXml(SecurityElement securityElement)
	{
		PermissionHelper.CheckSecurityElement(securityElement, "securityElement", 1, 1);
		if (securityElement.Tag != "IPermission")
		{
			throw new ArgumentException("securityElement");
		}
		if (PermissionHelper.IsUnrestricted(securityElement))
		{
			access = SmtpAccess.Connect;
		}
		else
		{
			access = SmtpAccess.None;
		}
	}

	public override IPermission Union(IPermission target)
	{
		SmtpPermission smtpPermission = Cast(target);
		if (smtpPermission == null)
		{
			return Copy();
		}
		if (unrestricted || smtpPermission.unrestricted)
		{
			return new SmtpPermission(unrestricted: true);
		}
		if (access > smtpPermission.access)
		{
			return new SmtpPermission(access);
		}
		return new SmtpPermission(smtpPermission.access);
	}

	private bool IsEmpty()
	{
		if (!unrestricted)
		{
			return access == SmtpAccess.None;
		}
		return false;
	}

	private SmtpPermission Cast(IPermission target)
	{
		if (target == null)
		{
			return null;
		}
		SmtpPermission obj = target as SmtpPermission;
		if (obj == null)
		{
			PermissionHelper.ThrowInvalidPermission(target, typeof(SmtpPermission));
		}
		return obj;
	}
}

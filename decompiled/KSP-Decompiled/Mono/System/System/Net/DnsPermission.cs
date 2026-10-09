using System.Security;
using System.Security.Permissions;

namespace System.Net;

[Serializable]
public sealed class DnsPermission : CodeAccessPermission, IUnrestrictedPermission
{
	private const int version = 1;

	private bool m_noRestriction;

	public DnsPermission(PermissionState state)
	{
		m_noRestriction = state == PermissionState.Unrestricted;
	}

	public override IPermission Copy()
	{
		return new DnsPermission(m_noRestriction ? PermissionState.Unrestricted : PermissionState.None);
	}

	public override IPermission Intersect(IPermission target)
	{
		DnsPermission dnsPermission = Cast(target);
		if (dnsPermission == null)
		{
			return null;
		}
		if (IsUnrestricted() && dnsPermission.IsUnrestricted())
		{
			return new DnsPermission(PermissionState.Unrestricted);
		}
		return null;
	}

	public override bool IsSubsetOf(IPermission target)
	{
		DnsPermission dnsPermission = Cast(target);
		if (dnsPermission == null)
		{
			return IsEmpty();
		}
		if (!dnsPermission.IsUnrestricted())
		{
			return m_noRestriction == dnsPermission.m_noRestriction;
		}
		return true;
	}

	public bool IsUnrestricted()
	{
		return m_noRestriction;
	}

	public override SecurityElement ToXml()
	{
		SecurityElement securityElement = PermissionHelper.Element(typeof(DnsPermission), 1);
		if (m_noRestriction)
		{
			securityElement.AddAttribute("Unrestricted", "true");
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
		m_noRestriction = PermissionHelper.IsUnrestricted(securityElement);
	}

	public override IPermission Union(IPermission target)
	{
		DnsPermission dnsPermission = Cast(target);
		if (dnsPermission == null)
		{
			return Copy();
		}
		if (IsUnrestricted() || dnsPermission.IsUnrestricted())
		{
			return new DnsPermission(PermissionState.Unrestricted);
		}
		return new DnsPermission(PermissionState.None);
	}

	private bool IsEmpty()
	{
		return !m_noRestriction;
	}

	private DnsPermission Cast(IPermission target)
	{
		if (target == null)
		{
			return null;
		}
		DnsPermission obj = target as DnsPermission;
		if (obj == null)
		{
			PermissionHelper.ThrowInvalidPermission(target, typeof(DnsPermission));
		}
		return obj;
	}
}

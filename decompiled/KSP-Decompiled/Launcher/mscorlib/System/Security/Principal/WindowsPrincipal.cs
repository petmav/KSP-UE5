using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security.Claims;
using System.Security.Permissions;
using Unity;

namespace System.Security.Principal;

[Serializable]
[ComVisible(true)]
public class WindowsPrincipal : ClaimsPrincipal
{
	private WindowsIdentity _identity;

	private string[] m_roles;

	public override IIdentity Identity => _identity;

	private IntPtr Token => _identity.Token;

	public virtual IEnumerable<Claim> DeviceClaims
	{
		get
		{
			//IL_0007: Expected O, but got I4
			ThrowStub.ThrowNotSupportedException();
			return (IEnumerable<Claim>)0;
		}
	}

	public virtual IEnumerable<Claim> UserClaims
	{
		get
		{
			//IL_0007: Expected O, but got I4
			ThrowStub.ThrowNotSupportedException();
			return (IEnumerable<Claim>)0;
		}
	}

	public WindowsPrincipal(WindowsIdentity ntIdentity)
	{
		if (ntIdentity == null)
		{
			throw new ArgumentNullException("ntIdentity");
		}
		_identity = ntIdentity;
	}

	public virtual bool IsInRole(int rid)
	{
		if (Environment.IsUnix)
		{
			return IsMemberOfGroupId(Token, (IntPtr)rid);
		}
		string text = null;
		switch (rid)
		{
		case 544:
			text = "BUILTIN\\Administrators";
			break;
		case 545:
			text = "BUILTIN\\Users";
			break;
		case 546:
			text = "BUILTIN\\Guests";
			break;
		case 547:
			text = "BUILTIN\\Power Users";
			break;
		case 548:
			text = "BUILTIN\\Account Operators";
			break;
		case 549:
			text = "BUILTIN\\System Operators";
			break;
		case 550:
			text = "BUILTIN\\Print Operators";
			break;
		case 551:
			text = "BUILTIN\\Backup Operators";
			break;
		case 552:
			text = "BUILTIN\\Replicator";
			break;
		default:
			return false;
		}
		return IsInRole(text);
	}

	[SecuritySafeCritical]
	[SecurityPermission(SecurityAction.Demand, ControlPrincipal = true)]
	public override bool IsInRole(string role)
	{
		if (role == null)
		{
			return false;
		}
		if (Environment.IsUnix)
		{
			return IsMemberOfGroupName(Token, role);
		}
		if (m_roles == null)
		{
			m_roles = WindowsIdentity._GetRoles(Token);
		}
		role = role.ToUpperInvariant();
		string[] roles = m_roles;
		foreach (string text in roles)
		{
			if (text != null && role == text.ToUpperInvariant())
			{
				return true;
			}
		}
		return false;
	}

	public virtual bool IsInRole(WindowsBuiltInRole role)
	{
		if (Environment.IsUnix)
		{
			string text = null;
			if (role == WindowsBuiltInRole.Administrator)
			{
				text = "root";
				return IsInRole(text);
			}
			return false;
		}
		return IsInRole((int)role);
	}

	[MonoTODO("not implemented")]
	[ComVisible(false)]
	public virtual bool IsInRole(SecurityIdentifier sid)
	{
		throw new NotImplementedException();
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern bool IsMemberOfGroupId(IntPtr user, IntPtr group);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern bool IsMemberOfGroupName(IntPtr user, string group);
}

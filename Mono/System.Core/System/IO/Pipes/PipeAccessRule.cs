using System.Security.AccessControl;
using System.Security.Permissions;
using System.Security.Principal;

namespace System.IO.Pipes;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public sealed class PipeAccessRule : AccessRule
{
	public PipeAccessRights PipeAccessRights => (PipeAccessRights)base.AccessMask;

	public PipeAccessRule(IdentityReference identity, PipeAccessRights rights, AccessControlType type)
		: base(identity, (int)rights, isInherited: false, InheritanceFlags.None, PropagationFlags.None, type)
	{
	}

	public PipeAccessRule(string identity, PipeAccessRights rights, AccessControlType type)
		: this(new NTAccount(identity), rights, type)
	{
	}
}

using System.Security.AccessControl;
using System.Security.Permissions;
using System.Security.Principal;

namespace System.IO.Pipes;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public sealed class PipeAuditRule : AuditRule
{
	public PipeAccessRights PipeAccessRights => (PipeAccessRights)base.AccessMask;

	public PipeAuditRule(IdentityReference identity, PipeAccessRights rights, AuditFlags flags)
		: base(identity, (int)rights, isInherited: false, InheritanceFlags.None, PropagationFlags.None, flags)
	{
	}

	public PipeAuditRule(string identity, PipeAccessRights rights, AuditFlags flags)
		: this(new NTAccount(identity), rights, flags)
	{
	}
}

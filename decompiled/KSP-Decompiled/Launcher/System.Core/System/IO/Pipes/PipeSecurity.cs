using System.Runtime.InteropServices;
using System.Security.AccessControl;
using System.Security.Permissions;
using System.Security.Principal;

namespace System.IO.Pipes;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class PipeSecurity : NativeObjectSecurity
{
	public override Type AccessRightType => typeof(PipeAccessRights);

	public override Type AccessRuleType => typeof(PipeAccessRule);

	public override Type AuditRuleType => typeof(PipeAuditRule);

	public PipeSecurity()
		: base(isContainer: false, ResourceType.FileObject)
	{
	}

	internal PipeSecurity(SafeHandle handle, AccessControlSections includeSections)
		: base(isContainer: false, ResourceType.FileObject, handle, includeSections)
	{
	}

	public override AccessRule AccessRuleFactory(IdentityReference identityReference, int accessMask, bool isInherited, InheritanceFlags inheritanceFlags, PropagationFlags propagationFlags, AccessControlType type)
	{
		return new PipeAccessRule(identityReference, (PipeAccessRights)accessMask, type);
	}

	public void AddAccessRule(PipeAccessRule rule)
	{
		AddAccessRule((AccessRule)rule);
	}

	public void AddAuditRule(PipeAuditRule rule)
	{
		AddAuditRule((AuditRule)rule);
	}

	public sealed override AuditRule AuditRuleFactory(IdentityReference identityReference, int accessMask, bool isInherited, InheritanceFlags inheritanceFlags, PropagationFlags propagationFlags, AuditFlags flags)
	{
		return new PipeAuditRule(identityReference, (PipeAccessRights)accessMask, flags);
	}

	[SecurityPermission(SecurityAction.Assert, UnmanagedCode = true)]
	protected internal void Persist(SafeHandle handle)
	{
		WriteLock();
		try
		{
			Persist(handle, base.AccessControlSectionsModified, null);
		}
		finally
		{
			WriteUnlock();
		}
	}

	[SecurityPermission(SecurityAction.Assert, UnmanagedCode = true)]
	protected internal void Persist(string name)
	{
		WriteLock();
		try
		{
			Persist(name, base.AccessControlSectionsModified, null);
		}
		finally
		{
			WriteUnlock();
		}
	}

	public bool RemoveAccessRule(PipeAccessRule rule)
	{
		return RemoveAccessRule((AccessRule)rule);
	}

	public void RemoveAccessRuleSpecific(PipeAccessRule rule)
	{
		RemoveAccessRuleSpecific((AccessRule)rule);
	}

	public bool RemoveAuditRule(PipeAuditRule rule)
	{
		return RemoveAuditRule((AuditRule)rule);
	}

	public void RemoveAuditRuleAll(PipeAuditRule rule)
	{
		RemoveAuditRuleAll((AuditRule)rule);
	}

	public void RemoveAuditRuleSpecific(PipeAuditRule rule)
	{
		RemoveAuditRuleSpecific((AuditRule)rule);
	}

	public void ResetAccessRule(PipeAccessRule rule)
	{
		ResetAccessRule((AccessRule)rule);
	}

	public void SetAccessRule(PipeAccessRule rule)
	{
		SetAccessRule((AccessRule)rule);
	}

	public void SetAuditRule(PipeAuditRule rule)
	{
		SetAuditRule((AuditRule)rule);
	}
}

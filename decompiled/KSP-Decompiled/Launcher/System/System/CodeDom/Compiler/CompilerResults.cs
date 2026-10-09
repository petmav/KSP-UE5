using System.Collections.Specialized;
using System.Reflection;
using System.Security.Permissions;
using System.Security.Policy;
using Unity;

namespace System.CodeDom.Compiler;

[Serializable]
[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
public class CompilerResults
{
	public Assembly CompiledAssembly
	{
		[SecurityPermission(SecurityAction.Assert, Flags = SecurityPermissionFlag.ControlEvidence)]
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public CompilerErrorCollection Errors
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public Evidence Evidence
	{
		[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
		[SecurityPermission(SecurityAction.Demand, ControlEvidence = true)]
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int NativeCompilerReturnValue
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public StringCollection Output
	{
		[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string PathToAssembly
	{
		[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public TempFileCollection TempFiles
	{
		[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
	public CompilerResults(TempFileCollection tempFiles)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

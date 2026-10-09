using System.Collections.Generic;
using System.Security.Permissions;
using Unity;

namespace System.CodeDom.Compiler;

[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public sealed class CompilerInfo
{
	public Type CodeDomProviderType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool IsCodeDomProviderTypeValid
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	internal CompilerInfo()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CompilerParameters CreateDefaultCompilerParameters()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public CodeDomProvider CreateProvider()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public CodeDomProvider CreateProvider(IDictionary<string, string> providerOptions)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public string[] GetExtensions()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public string[] GetLanguages()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

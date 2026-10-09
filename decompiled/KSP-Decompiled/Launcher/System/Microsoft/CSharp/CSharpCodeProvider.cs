using System;
using System.CodeDom;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Security.Permissions;
using Unity;

namespace Microsoft.CSharp;

[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public class CSharpCodeProvider : CodeDomProvider
{
	public override string FileExtension
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CSharpCodeProvider()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CSharpCodeProvider(IDictionary<string, string> providerOptions)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[Obsolete("Callers should not use the ICodeCompiler interface and should instead use the methods directly on the CodeDomProvider class.")]
	public override ICodeCompiler CreateCompiler()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[Obsolete("Callers should not use the ICodeGenerator interface and should instead use the methods directly on the CodeDomProvider class.")]
	public override ICodeGenerator CreateGenerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override void GenerateCodeFromMember(CodeTypeMember member, TextWriter writer, CodeGeneratorOptions options)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override TypeConverter GetConverter(Type type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

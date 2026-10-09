using System;
using System.CodeDom;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Reflection;

namespace Microsoft.CSharp;

public class CSharpCodeProvider : CodeDomProvider
{
	private readonly CSharpCodeGenerator _generator;

	public override string FileExtension => "cs";

	public CSharpCodeProvider()
	{
		_generator = new CSharpCodeGenerator();
	}

	public CSharpCodeProvider(IDictionary<string, string> providerOptions)
	{
		if (providerOptions == null)
		{
			throw new ArgumentNullException("providerOptions");
		}
		_generator = new CSharpCodeGenerator(providerOptions);
	}

	[Obsolete("Callers should not use the ICodeGenerator interface and should instead use the methods directly on the CodeDomProvider class.")]
	public override ICodeGenerator CreateGenerator()
	{
		return _generator;
	}

	[Obsolete("Callers should not use the ICodeCompiler interface and should instead use the methods directly on the CodeDomProvider class.")]
	public override ICodeCompiler CreateCompiler()
	{
		return _generator;
	}

	public override TypeConverter GetConverter(Type type)
	{
		if (!(type == typeof(MemberAttributes)))
		{
			if (!(type == typeof(TypeAttributes)))
			{
				return base.GetConverter(type);
			}
			return CSharpTypeAttributeConverter.Default;
		}
		return CSharpMemberAttributeConverter.Default;
	}

	public override void GenerateCodeFromMember(CodeTypeMember member, TextWriter writer, CodeGeneratorOptions options)
	{
		_generator.GenerateCodeFromMember(member, writer, options);
	}
}

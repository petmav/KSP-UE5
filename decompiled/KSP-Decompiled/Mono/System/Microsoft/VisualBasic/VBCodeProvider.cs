using System;
using System.CodeDom;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Reflection;

namespace Microsoft.VisualBasic;

public class VBCodeProvider : CodeDomProvider
{
	private VBCodeGenerator _generator;

	public override string FileExtension => "vb";

	public override LanguageOptions LanguageOptions => LanguageOptions.CaseInsensitive;

	public VBCodeProvider()
	{
		_generator = new VBCodeGenerator();
	}

	public VBCodeProvider(IDictionary<string, string> providerOptions)
	{
		if (providerOptions == null)
		{
			throw new ArgumentNullException("providerOptions");
		}
		_generator = new VBCodeGenerator(providerOptions);
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
			return VBTypeAttributeConverter.Default;
		}
		return VBMemberAttributeConverter.Default;
	}

	public override void GenerateCodeFromMember(CodeTypeMember member, TextWriter writer, CodeGeneratorOptions options)
	{
		_generator.GenerateCodeFromMember(member, writer, options);
	}
}

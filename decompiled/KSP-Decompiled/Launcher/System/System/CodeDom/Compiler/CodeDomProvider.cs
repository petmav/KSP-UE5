using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Permissions;
using Unity;

namespace System.CodeDom.Compiler;

[ComVisible(true)]
[ToolboxItem(false)]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
public abstract class CodeDomProvider : Component
{
	public virtual string FileExtension
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public virtual LanguageOptions LanguageOptions
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(LanguageOptions);
		}
	}

	protected CodeDomProvider()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual CompilerResults CompileAssemblyFromDom(CompilerParameters options, CodeCompileUnit[] compilationUnits)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual CompilerResults CompileAssemblyFromFile(CompilerParameters options, string[] fileNames)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual CompilerResults CompileAssemblyFromSource(CompilerParameters options, string[] sources)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[Obsolete("Callers should not use the ICodeCompiler interface and should instead use the methods directly on the CodeDomProvider class. Those inheriting from CodeDomProvider must still implement this interface, and should exclude this warning or also obsolete this method.")]
	public abstract ICodeCompiler CreateCompiler();

	public virtual string CreateEscapedIdentifier(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[Obsolete("Callers should not use the ICodeGenerator interface and should instead use the methods directly on the CodeDomProvider class. Those inheriting from CodeDomProvider must still implement this interface, and should exclude this warning or also obsolete this method.")]
	public abstract ICodeGenerator CreateGenerator();

	public virtual ICodeGenerator CreateGenerator(TextWriter output)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual ICodeGenerator CreateGenerator(string fileName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[Obsolete("Callers should not use the ICodeParser interface and should instead use the methods directly on the CodeDomProvider class. Those inheriting from CodeDomProvider must still implement this interface, and should exclude this warning or also obsolete this method.")]
	public virtual ICodeParser CreateParser()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[ComVisible(false)]
	public static CodeDomProvider CreateProvider(string language)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[ComVisible(false)]
	public static CodeDomProvider CreateProvider(string language, IDictionary<string, string> providerOptions)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual string CreateValidIdentifier(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual void GenerateCodeFromCompileUnit(CodeCompileUnit compileUnit, TextWriter writer, CodeGeneratorOptions options)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void GenerateCodeFromExpression(CodeExpression expression, TextWriter writer, CodeGeneratorOptions options)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void GenerateCodeFromMember(CodeTypeMember member, TextWriter writer, CodeGeneratorOptions options)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void GenerateCodeFromNamespace(CodeNamespace codeNamespace, TextWriter writer, CodeGeneratorOptions options)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void GenerateCodeFromStatement(CodeStatement statement, TextWriter writer, CodeGeneratorOptions options)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void GenerateCodeFromType(CodeTypeDeclaration codeType, TextWriter writer, CodeGeneratorOptions options)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[ComVisible(false)]
	public static CompilerInfo[] GetAllCompilerInfo()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[ComVisible(false)]
	public static CompilerInfo GetCompilerInfo(string language)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual TypeConverter GetConverter(Type type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[ComVisible(false)]
	public static string GetLanguageFromExtension(string extension)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual string GetTypeOutput(CodeTypeReference type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[ComVisible(false)]
	public static bool IsDefinedExtension(string extension)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[ComVisible(false)]
	public static bool IsDefinedLanguage(string language)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual bool IsValidIdentifier(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual CodeCompileUnit Parse(TextReader codeStream)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual bool Supports(GeneratorSupport generatorSupport)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}

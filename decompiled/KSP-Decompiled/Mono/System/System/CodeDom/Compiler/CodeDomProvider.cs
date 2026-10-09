using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Runtime.Serialization;
using Microsoft.CSharp;
using Microsoft.VisualBasic;

namespace System.CodeDom.Compiler;

public abstract class CodeDomProvider : Component
{
	private sealed class ConfigurationErrorsException : SystemException
	{
		public ConfigurationErrorsException(string message)
			: base(message)
		{
		}

		public ConfigurationErrorsException(SerializationInfo info, StreamingContext context)
			: base(info, context)
		{
			throw new PlatformNotSupportedException();
		}
	}

	private static readonly Dictionary<string, CompilerInfo> s_compilerLanguages;

	private static readonly Dictionary<string, CompilerInfo> s_compilerExtensions;

	private static readonly List<CompilerInfo> s_allCompilerInfo;

	public virtual string FileExtension => string.Empty;

	public virtual LanguageOptions LanguageOptions => LanguageOptions.None;

	static CodeDomProvider()
	{
		s_compilerLanguages = new Dictionary<string, CompilerInfo>(StringComparer.OrdinalIgnoreCase);
		s_compilerExtensions = new Dictionary<string, CompilerInfo>(StringComparer.OrdinalIgnoreCase);
		s_allCompilerInfo = new List<CompilerInfo>();
		AddCompilerInfo(new CompilerInfo(new CompilerParameters
		{
			WarningLevel = 4
		}, typeof(CSharpCodeProvider).FullName)
		{
			_compilerLanguages = new string[3] { "c#", "cs", "csharp" },
			_compilerExtensions = new string[2] { ".cs", "cs" }
		});
		AddCompilerInfo(new CompilerInfo(new CompilerParameters
		{
			WarningLevel = 4
		}, typeof(VBCodeProvider).FullName)
		{
			_compilerLanguages = new string[4] { "vb", "vbs", "visualbasic", "vbscript" },
			_compilerExtensions = new string[2] { ".vb", "vb" }
		});
	}

	private static void AddCompilerInfo(CompilerInfo compilerInfo)
	{
		string[] compilerLanguages = compilerInfo._compilerLanguages;
		foreach (string key in compilerLanguages)
		{
			s_compilerLanguages[key] = compilerInfo;
		}
		compilerLanguages = compilerInfo._compilerExtensions;
		foreach (string key2 in compilerLanguages)
		{
			s_compilerExtensions[key2] = compilerInfo;
		}
		s_allCompilerInfo.Add(compilerInfo);
	}

	public static CodeDomProvider CreateProvider(string language, IDictionary<string, string> providerOptions)
	{
		return GetCompilerInfo(language).CreateProvider(providerOptions);
	}

	public static CodeDomProvider CreateProvider(string language)
	{
		return GetCompilerInfo(language).CreateProvider();
	}

	public static string GetLanguageFromExtension(string extension)
	{
		return (GetCompilerInfoForExtensionNoThrow(extension) ?? throw new ConfigurationErrorsException("There is no CodeDom provider defined for the language."))._compilerLanguages[0];
	}

	public static bool IsDefinedLanguage(string language)
	{
		return GetCompilerInfoForLanguageNoThrow(language) != null;
	}

	public static bool IsDefinedExtension(string extension)
	{
		return GetCompilerInfoForExtensionNoThrow(extension) != null;
	}

	public static CompilerInfo GetCompilerInfo(string language)
	{
		return GetCompilerInfoForLanguageNoThrow(language) ?? throw new ConfigurationErrorsException("There is no CodeDom provider defined for the language.");
	}

	private static CompilerInfo GetCompilerInfoForLanguageNoThrow(string language)
	{
		if (language == null)
		{
			throw new ArgumentNullException("language");
		}
		s_compilerLanguages.TryGetValue(language.Trim(), out var value);
		return value;
	}

	private static CompilerInfo GetCompilerInfoForExtensionNoThrow(string extension)
	{
		if (extension == null)
		{
			throw new ArgumentNullException("extension");
		}
		s_compilerExtensions.TryGetValue(extension.Trim(), out var value);
		return value;
	}

	public static CompilerInfo[] GetAllCompilerInfo()
	{
		return s_allCompilerInfo.ToArray();
	}

	[Obsolete("Callers should not use the ICodeGenerator interface and should instead use the methods directly on the CodeDomProvider class. Those inheriting from CodeDomProvider must still implement this interface, and should exclude this warning or also obsolete this method.")]
	public abstract ICodeGenerator CreateGenerator();

	public virtual ICodeGenerator CreateGenerator(TextWriter output)
	{
		return CreateGenerator();
	}

	public virtual ICodeGenerator CreateGenerator(string fileName)
	{
		return CreateGenerator();
	}

	[Obsolete("Callers should not use the ICodeCompiler interface and should instead use the methods directly on the CodeDomProvider class. Those inheriting from CodeDomProvider must still implement this interface, and should exclude this warning or also obsolete this method.")]
	public abstract ICodeCompiler CreateCompiler();

	[Obsolete("Callers should not use the ICodeParser interface and should instead use the methods directly on the CodeDomProvider class. Those inheriting from CodeDomProvider must still implement this interface, and should exclude this warning or also obsolete this method.")]
	public virtual ICodeParser CreateParser()
	{
		return null;
	}

	public virtual TypeConverter GetConverter(Type type)
	{
		return TypeDescriptor.GetConverter(type);
	}

	public virtual CompilerResults CompileAssemblyFromDom(CompilerParameters options, params CodeCompileUnit[] compilationUnits)
	{
		return CreateCompilerHelper().CompileAssemblyFromDomBatch(options, compilationUnits);
	}

	public virtual CompilerResults CompileAssemblyFromFile(CompilerParameters options, params string[] fileNames)
	{
		return CreateCompilerHelper().CompileAssemblyFromFileBatch(options, fileNames);
	}

	public virtual CompilerResults CompileAssemblyFromSource(CompilerParameters options, params string[] sources)
	{
		return CreateCompilerHelper().CompileAssemblyFromSourceBatch(options, sources);
	}

	public virtual bool IsValidIdentifier(string value)
	{
		return CreateGeneratorHelper().IsValidIdentifier(value);
	}

	public virtual string CreateEscapedIdentifier(string value)
	{
		return CreateGeneratorHelper().CreateEscapedIdentifier(value);
	}

	public virtual string CreateValidIdentifier(string value)
	{
		return CreateGeneratorHelper().CreateValidIdentifier(value);
	}

	public virtual string GetTypeOutput(CodeTypeReference type)
	{
		return CreateGeneratorHelper().GetTypeOutput(type);
	}

	public virtual bool Supports(GeneratorSupport generatorSupport)
	{
		return CreateGeneratorHelper().Supports(generatorSupport);
	}

	public virtual void GenerateCodeFromExpression(CodeExpression expression, TextWriter writer, CodeGeneratorOptions options)
	{
		CreateGeneratorHelper().GenerateCodeFromExpression(expression, writer, options);
	}

	public virtual void GenerateCodeFromStatement(CodeStatement statement, TextWriter writer, CodeGeneratorOptions options)
	{
		CreateGeneratorHelper().GenerateCodeFromStatement(statement, writer, options);
	}

	public virtual void GenerateCodeFromNamespace(CodeNamespace codeNamespace, TextWriter writer, CodeGeneratorOptions options)
	{
		CreateGeneratorHelper().GenerateCodeFromNamespace(codeNamespace, writer, options);
	}

	public virtual void GenerateCodeFromCompileUnit(CodeCompileUnit compileUnit, TextWriter writer, CodeGeneratorOptions options)
	{
		CreateGeneratorHelper().GenerateCodeFromCompileUnit(compileUnit, writer, options);
	}

	public virtual void GenerateCodeFromType(CodeTypeDeclaration codeType, TextWriter writer, CodeGeneratorOptions options)
	{
		CreateGeneratorHelper().GenerateCodeFromType(codeType, writer, options);
	}

	public virtual void GenerateCodeFromMember(CodeTypeMember member, TextWriter writer, CodeGeneratorOptions options)
	{
		throw new NotImplementedException("This CodeDomProvider does not support this method.");
	}

	public virtual CodeCompileUnit Parse(TextReader codeStream)
	{
		return CreateParserHelper().Parse(codeStream);
	}

	private ICodeCompiler CreateCompilerHelper()
	{
		return CreateCompiler() ?? throw new NotImplementedException("This CodeDomProvider does not support this method.");
	}

	private ICodeGenerator CreateGeneratorHelper()
	{
		return CreateGenerator() ?? throw new NotImplementedException("This CodeDomProvider does not support this method.");
	}

	private ICodeParser CreateParserHelper()
	{
		return CreateParser() ?? throw new NotImplementedException("This CodeDomProvider does not support this method.");
	}
}

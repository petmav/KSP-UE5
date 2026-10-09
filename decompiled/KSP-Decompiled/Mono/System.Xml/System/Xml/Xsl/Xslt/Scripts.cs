using System.CodeDom;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Configuration;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security;
using System.Security.Permissions;
using System.Text.RegularExpressions;
using System.Xml.Xsl.Runtime;
using Microsoft.VisualBasic;

namespace System.Xml.Xsl.Xslt;

internal class Scripts
{
	private const string ScriptClassesNamespace = "System.Xml.Xsl.CompiledQuery";

	private Compiler compiler;

	private List<ScriptClass> scriptClasses = new List<ScriptClass>();

	private Dictionary<string, Type> nsToType = new Dictionary<string, Type>();

	private XmlExtensionFunctionTable extFuncs = new XmlExtensionFunctionTable();

	private static readonly string[] defaultNamespaces = new string[7] { "System", "System.Collections", "System.Text", "System.Text.RegularExpressions", "System.Xml", "System.Xml.Xsl", "System.Xml.XPath" };

	private int assemblyCounter;

	public Dictionary<string, Type> ScriptClasses => nsToType;

	public Scripts(Compiler compiler)
	{
		this.compiler = compiler;
	}

	public XmlExtensionFunction ResolveFunction(string name, string ns, int numArgs, IErrorHelper errorHelper)
	{
		if (nsToType.TryGetValue(ns, out var value))
		{
			try
			{
				return extFuncs.Bind(name, ns, numArgs, value, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public);
			}
			catch (XslTransformException ex)
			{
				errorHelper.ReportError(ex.Message);
			}
		}
		return null;
	}

	public ScriptClass GetScriptClass(string ns, string language, IErrorHelper errorHelper)
	{
		CompilerInfo compilerInfo;
		try
		{
			compilerInfo = CodeDomProvider.GetCompilerInfo(language);
		}
		catch (ConfigurationException)
		{
			errorHelper.ReportError("Scripting language '{0}' is not supported.", language);
			return null;
		}
		foreach (ScriptClass scriptClass2 in scriptClasses)
		{
			if (ns == scriptClass2.ns)
			{
				if (compilerInfo != scriptClass2.compilerInfo)
				{
					errorHelper.ReportError("All script blocks implementing the namespace '{0}' must use the same language.", ns);
					return null;
				}
				return scriptClass2;
			}
		}
		ScriptClass scriptClass = new ScriptClass(ns, compilerInfo);
		scriptClass.typeDecl.TypeAttributes = TypeAttributes.Public;
		scriptClasses.Add(scriptClass);
		return scriptClass;
	}

	public void CompileScripts()
	{
		List<ScriptClass> list = new List<ScriptClass>();
		for (int i = 0; i < scriptClasses.Count; i++)
		{
			if (scriptClasses[i] == null)
			{
				continue;
			}
			CompilerInfo compilerInfo = scriptClasses[i].compilerInfo;
			list.Clear();
			for (int j = i; j < scriptClasses.Count; j++)
			{
				if (scriptClasses[j] != null && scriptClasses[j].compilerInfo == compilerInfo)
				{
					list.Add(scriptClasses[j]);
					scriptClasses[j] = null;
				}
			}
			Assembly assembly = CompileAssembly(list);
			if (!(assembly != null))
			{
				continue;
			}
			foreach (ScriptClass item in list)
			{
				Type type = assembly.GetType("System.Xml.Xsl.CompiledQuery" + Type.Delimiter + item.typeDecl.Name);
				if (type != null)
				{
					nsToType.Add(item.ns, type);
				}
			}
		}
	}

	[PermissionSet(SecurityAction.Demand, Name = "FullTrust")]
	private Assembly CompileAssembly(List<ScriptClass> scriptsForLang)
	{
		TempFileCollection tempFiles = compiler.CompilerResults.TempFiles;
		CompilerErrorCollection errors = compiler.CompilerResults.Errors;
		ScriptClass scriptClass = scriptsForLang[scriptsForLang.Count - 1];
		bool flag = false;
		CodeDomProvider codeDomProvider;
		try
		{
			codeDomProvider = scriptClass.compilerInfo.CreateProvider();
		}
		catch (ConfigurationException ex)
		{
			errors.Add(compiler.CreateError(scriptClass.EndLineInfo, "Error occurred while compiling the script: {0}", ex.Message));
			return null;
		}
		flag = codeDomProvider is VBCodeProvider;
		CodeCompileUnit[] array = new CodeCompileUnit[scriptsForLang.Count];
		CompilerParameters compilerParameters = scriptClass.compilerInfo.CreateDefaultCompilerParameters();
		compilerParameters.ReferencedAssemblies.Add(typeof(Res).Assembly.Location);
		compilerParameters.ReferencedAssemblies.Add("System.dll");
		if (flag)
		{
			compilerParameters.ReferencedAssemblies.Add("Microsoft.VisualBasic.dll");
		}
		bool flag2 = false;
		for (int i = 0; i < scriptsForLang.Count; i++)
		{
			ScriptClass scriptClass2 = scriptsForLang[i];
			CodeNamespace codeNamespace = new CodeNamespace("System.Xml.Xsl.CompiledQuery");
			string[] array2 = defaultNamespaces;
			foreach (string nameSpace in array2)
			{
				codeNamespace.Imports.Add(new CodeNamespaceImport(nameSpace));
			}
			if (flag)
			{
				codeNamespace.Imports.Add(new CodeNamespaceImport("Microsoft.VisualBasic"));
			}
			StringEnumerator enumerator = scriptClass2.nsImports.GetEnumerator();
			try
			{
				while (enumerator.MoveNext())
				{
					string current = enumerator.Current;
					codeNamespace.Imports.Add(new CodeNamespaceImport(current));
				}
			}
			finally
			{
				if (enumerator is IDisposable disposable)
				{
					disposable.Dispose();
				}
			}
			codeNamespace.Types.Add(scriptClass2.typeDecl);
			CodeCompileUnit codeCompileUnit = new CodeCompileUnit();
			codeCompileUnit.Namespaces.Add(codeNamespace);
			if (flag)
			{
				codeCompileUnit.UserData["AllowLateBound"] = true;
				codeCompileUnit.UserData["RequireVariableDeclaration"] = false;
			}
			if (i == 0)
			{
				codeCompileUnit.AssemblyCustomAttributes.Add(new CodeAttributeDeclaration("System.Security.SecurityTransparentAttribute"));
				codeCompileUnit.AssemblyCustomAttributes.Add(new CodeAttributeDeclaration(new CodeTypeReference(typeof(SecurityRulesAttribute)), new CodeAttributeArgument(new CodeFieldReferenceExpression(new CodeTypeReferenceExpression(typeof(SecurityRuleSet)), "Level1"))));
			}
			array[i] = codeCompileUnit;
			enumerator = scriptClass2.refAssemblies.GetEnumerator();
			try
			{
				while (enumerator.MoveNext())
				{
					string current2 = enumerator.Current;
					compilerParameters.ReferencedAssemblies.Add(current2);
				}
			}
			finally
			{
				if (enumerator is IDisposable disposable2)
				{
					disposable2.Dispose();
				}
			}
			flag2 |= scriptClass2.refAssembliesByHref;
		}
		XsltSettings settings = compiler.Settings;
		compilerParameters.WarningLevel = ((settings.WarningLevel >= 0) ? settings.WarningLevel : compilerParameters.WarningLevel);
		compilerParameters.TreatWarningsAsErrors = settings.TreatWarningsAsErrors;
		compilerParameters.IncludeDebugInformation = compiler.IsDebug;
		string text = compiler.ScriptAssemblyPath;
		if (text != null && scriptsForLang.Count < scriptClasses.Count)
		{
			text = Path.ChangeExtension(text, "." + GetLanguageName(scriptClass.compilerInfo) + Path.GetExtension(text));
		}
		compilerParameters.OutputAssembly = text;
		string tempDir = ((settings.TempFiles != null) ? settings.TempFiles.TempDir : null);
		compilerParameters.TempFiles = new TempFileCollection(tempDir);
		bool keepFiles = compiler.IsDebug && text == null && !settings.CheckOnly;
		compilerParameters.TempFiles.KeepFiles = keepFiles;
		compilerParameters.GenerateInMemory = (text == null && !compiler.IsDebug && !flag2) || settings.CheckOnly;
		CompilerResults compilerResults;
		try
		{
			compilerResults = codeDomProvider.CompileAssemblyFromDom(compilerParameters, array);
		}
		catch (ExternalException ex2)
		{
			compilerResults = new CompilerResults(compilerParameters.TempFiles);
			compilerResults.Errors.Add(compiler.CreateError(scriptClass.EndLineInfo, "Error occurred while compiling the script: {0}", ex2.Message));
		}
		if (!settings.CheckOnly)
		{
			foreach (string tempFile in compilerResults.TempFiles)
			{
				tempFiles.AddFile(tempFile, tempFiles.KeepFiles);
			}
		}
		foreach (CompilerError error in compilerResults.Errors)
		{
			FixErrorPosition(error, scriptsForLang);
			compiler.AddModule(error.FileName);
		}
		errors.AddRange(compilerResults.Errors);
		if (!compilerResults.Errors.HasErrors)
		{
			return compilerResults.CompiledAssembly;
		}
		return null;
	}

	private string GetLanguageName(CompilerInfo compilerInfo)
	{
		Regex regex = new Regex("^[0-9a-zA-Z]+$");
		string[] languages = compilerInfo.GetLanguages();
		foreach (string text in languages)
		{
			if (regex.IsMatch(text))
			{
				return text;
			}
		}
		int i = ++assemblyCounter;
		return "script" + i.ToString(CultureInfo.InvariantCulture);
	}

	private static void FixErrorPosition(CompilerError error, List<ScriptClass> scriptsForLang)
	{
		string fileName = error.FileName;
		foreach (ScriptClass item in scriptsForLang)
		{
			if (item.scriptUris.TryGetValue(fileName, out var value))
			{
				error.FileName = value;
				return;
			}
		}
		ScriptClass scriptClass = scriptsForLang[scriptsForLang.Count - 1];
		fileName = Path.GetFileNameWithoutExtension(fileName);
		int num;
		if ((num = fileName.LastIndexOf('.')) >= 0 && int.TryParse(fileName.Substring(num + 1), NumberStyles.None, NumberFormatInfo.InvariantInfo, out var result) && (uint)result < scriptsForLang.Count)
		{
			scriptClass = scriptsForLang[result];
		}
		error.FileName = scriptClass.endUri;
		error.Line = scriptClass.endLoc.Line;
		error.Column = scriptClass.endLoc.Pos;
	}
}

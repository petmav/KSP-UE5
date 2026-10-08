using System.CodeDom.Compiler;
using System.IO;
using System.Reflection;
using System.Reflection.Emit;
using System.Security;
using System.Security.Permissions;
using System.Xml.XmlConfiguration;
using System.Xml.XPath;
using System.Xml.Xsl.Qil;
using System.Xml.Xsl.Runtime;
using System.Xml.Xsl.Xslt;

namespace System.Xml.Xsl;

public sealed class XslCompiledTransform
{
	private static readonly XmlReaderSettings ReaderSettings;

	private static readonly PermissionSet MemberAccessPermissionSet;

	private const string Version = "4.0.0.0";

	private bool enableDebug;

	private CompilerResults compilerResults;

	private XmlWriterSettings outputSettings;

	private QilExpression qil;

	private XmlILCommand command;

	private static volatile ConstructorInfo GeneratedCodeCtor;

	internal CompilerErrorCollection Errors
	{
		get
		{
			if (compilerResults == null)
			{
				return null;
			}
			return compilerResults.Errors;
		}
	}

	public XmlWriterSettings OutputSettings => outputSettings;

	public TempFileCollection TemporaryFiles
	{
		[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
		get
		{
			if (compilerResults == null)
			{
				return null;
			}
			return compilerResults.TempFiles;
		}
	}

	static XslCompiledTransform()
	{
		MemberAccessPermissionSet = new PermissionSet(PermissionState.None);
		MemberAccessPermissionSet.AddPermission(new ReflectionPermission(ReflectionPermissionFlag.MemberAccess));
		ReaderSettings = new XmlReaderSettings();
	}

	public XslCompiledTransform()
	{
	}

	public XslCompiledTransform(bool enableDebug)
	{
		this.enableDebug = enableDebug;
	}

	private void Reset()
	{
		compilerResults = null;
		outputSettings = null;
		qil = null;
		command = null;
	}

	public void Load(XmlReader stylesheet)
	{
		Reset();
		LoadInternal(stylesheet, XsltSettings.Default, XsltConfigSection.CreateDefaultResolver());
	}

	public void Load(XmlReader stylesheet, XsltSettings settings, XmlResolver stylesheetResolver)
	{
		Reset();
		LoadInternal(stylesheet, settings, stylesheetResolver);
	}

	public void Load(IXPathNavigable stylesheet)
	{
		Reset();
		LoadInternal(stylesheet, XsltSettings.Default, XsltConfigSection.CreateDefaultResolver());
	}

	public void Load(IXPathNavigable stylesheet, XsltSettings settings, XmlResolver stylesheetResolver)
	{
		Reset();
		LoadInternal(stylesheet, settings, stylesheetResolver);
	}

	public void Load(string stylesheetUri)
	{
		Reset();
		if (stylesheetUri == null)
		{
			throw new ArgumentNullException("stylesheetUri");
		}
		LoadInternal(stylesheetUri, XsltSettings.Default, XsltConfigSection.CreateDefaultResolver());
	}

	public void Load(string stylesheetUri, XsltSettings settings, XmlResolver stylesheetResolver)
	{
		Reset();
		if (stylesheetUri == null)
		{
			throw new ArgumentNullException("stylesheetUri");
		}
		LoadInternal(stylesheetUri, settings, stylesheetResolver);
	}

	private CompilerResults LoadInternal(object stylesheet, XsltSettings settings, XmlResolver stylesheetResolver)
	{
		if (stylesheet == null)
		{
			throw new ArgumentNullException("stylesheet");
		}
		if (settings == null)
		{
			settings = XsltSettings.Default;
		}
		CompileXsltToQil(stylesheet, settings, stylesheetResolver);
		CompilerError firstError = GetFirstError();
		if (firstError != null)
		{
			throw new XslLoadException(firstError);
		}
		if (!settings.CheckOnly)
		{
			CompileQilToMsil(settings);
		}
		return compilerResults;
	}

	private void CompileXsltToQil(object stylesheet, XsltSettings settings, XmlResolver stylesheetResolver)
	{
		compilerResults = new Compiler(settings, enableDebug, null).Compile(stylesheet, stylesheetResolver, out qil);
	}

	private CompilerError GetFirstError()
	{
		foreach (CompilerError error in compilerResults.Errors)
		{
			if (!error.IsWarning)
			{
				return error;
			}
		}
		return null;
	}

	private void CompileQilToMsil(XsltSettings settings)
	{
		command = new XmlILGenerator().Generate(qil, null);
		outputSettings = command.StaticData.DefaultWriterSettings;
		qil = null;
	}

	public static CompilerErrorCollection CompileToType(XmlReader stylesheet, XsltSettings settings, XmlResolver stylesheetResolver, bool debug, TypeBuilder typeBuilder, string scriptAssemblyPath)
	{
		if (stylesheet == null)
		{
			throw new ArgumentNullException("stylesheet");
		}
		if (typeBuilder == null)
		{
			throw new ArgumentNullException("typeBuilder");
		}
		if (settings == null)
		{
			settings = XsltSettings.Default;
		}
		if (settings.EnableScript && scriptAssemblyPath == null)
		{
			throw new ArgumentNullException("scriptAssemblyPath");
		}
		if (scriptAssemblyPath != null)
		{
			scriptAssemblyPath = Path.GetFullPath(scriptAssemblyPath);
		}
		QilExpression query;
		CompilerErrorCollection errors = new Compiler(settings, debug, scriptAssemblyPath).Compile(stylesheet, stylesheetResolver, out query).Errors;
		if (!errors.HasErrors)
		{
			if (GeneratedCodeCtor == null)
			{
				GeneratedCodeCtor = typeof(GeneratedCodeAttribute).GetConstructor(new Type[2]
				{
					typeof(string),
					typeof(string)
				});
			}
			typeBuilder.SetCustomAttribute(new CustomAttributeBuilder(GeneratedCodeCtor, new object[2]
			{
				typeof(XslCompiledTransform).FullName,
				"4.0.0.0"
			}));
			new XmlILGenerator().Generate(query, typeBuilder);
		}
		return errors;
	}

	public void Load(Type compiledStylesheet)
	{
		Reset();
		if (compiledStylesheet == null)
		{
			throw new ArgumentNullException("compiledStylesheet");
		}
		object[] customAttributes = compiledStylesheet.GetCustomAttributes(typeof(GeneratedCodeAttribute), inherit: false);
		GeneratedCodeAttribute generatedCodeAttribute = ((customAttributes.Length != 0) ? ((GeneratedCodeAttribute)customAttributes[0]) : null);
		if (generatedCodeAttribute != null && generatedCodeAttribute.Tool == typeof(XslCompiledTransform).FullName)
		{
			if (new Version("4.0.0.0").CompareTo(new Version(generatedCodeAttribute.Version)) < 0)
			{
				throw new ArgumentException(Res.GetString("Executing a stylesheet that was compiled using a later version of the framework is not supported. Stylesheet Version: {0}. Current Framework Version: {1}.", generatedCodeAttribute.Version, "4.0.0.0"), "compiledStylesheet");
			}
			FieldInfo field = compiledStylesheet.GetField("staticData", BindingFlags.Static | BindingFlags.NonPublic);
			FieldInfo field2 = compiledStylesheet.GetField("ebTypes", BindingFlags.Static | BindingFlags.NonPublic);
			if (field != null && field2 != null)
			{
				if (XsltConfigSection.EnableMemberAccessForXslCompiledTransform)
				{
					new ReflectionPermission(ReflectionPermissionFlag.MemberAccess).Assert();
				}
				if (field.GetValue(null) is byte[] queryData)
				{
					MethodInfo method = compiledStylesheet.GetMethod("Execute", BindingFlags.Static | BindingFlags.NonPublic);
					Type[] earlyBoundTypes = (Type[])field2.GetValue(null);
					Load(method, queryData, earlyBoundTypes);
					return;
				}
			}
		}
		if (command == null)
		{
			throw new ArgumentException(Res.GetString("Type '{0}' is not a compiled stylesheet class.", compiledStylesheet.FullName), "compiledStylesheet");
		}
	}

	public void Load(MethodInfo executeMethod, byte[] queryData, Type[] earlyBoundTypes)
	{
		Reset();
		if (executeMethod == null)
		{
			throw new ArgumentNullException("executeMethod");
		}
		if (queryData == null)
		{
			throw new ArgumentNullException("queryData");
		}
		if (!XsltConfigSection.EnableMemberAccessForXslCompiledTransform && executeMethod.DeclaringType != null && !executeMethod.DeclaringType.IsVisible)
		{
			new ReflectionPermission(ReflectionPermissionFlag.MemberAccess).Demand();
		}
		DynamicMethod dynamicMethod = executeMethod as DynamicMethod;
		Delegate @delegate = ((dynamicMethod != null) ? dynamicMethod.CreateDelegate(typeof(ExecuteDelegate)) : Delegate.CreateDelegate(typeof(ExecuteDelegate), executeMethod));
		command = new XmlILCommand((ExecuteDelegate)@delegate, new XmlQueryStaticData(queryData, earlyBoundTypes));
		outputSettings = command.StaticData.DefaultWriterSettings;
	}

	public void Transform(IXPathNavigable input, XmlWriter results)
	{
		CheckArguments(input, results);
		Transform(input, null, results, XsltConfigSection.CreateDefaultResolver());
	}

	public void Transform(IXPathNavigable input, XsltArgumentList arguments, XmlWriter results)
	{
		CheckArguments(input, results);
		Transform(input, arguments, results, XsltConfigSection.CreateDefaultResolver());
	}

	public void Transform(IXPathNavigable input, XsltArgumentList arguments, TextWriter results)
	{
		CheckArguments(input, results);
		using XmlWriter xmlWriter = XmlWriter.Create(results, OutputSettings);
		Transform(input, arguments, xmlWriter, XsltConfigSection.CreateDefaultResolver());
		xmlWriter.Close();
	}

	public void Transform(IXPathNavigable input, XsltArgumentList arguments, Stream results)
	{
		CheckArguments(input, results);
		using XmlWriter xmlWriter = XmlWriter.Create(results, OutputSettings);
		Transform(input, arguments, xmlWriter, XsltConfigSection.CreateDefaultResolver());
		xmlWriter.Close();
	}

	public void Transform(XmlReader input, XmlWriter results)
	{
		CheckArguments(input, results);
		Transform(input, null, results, XsltConfigSection.CreateDefaultResolver());
	}

	public void Transform(XmlReader input, XsltArgumentList arguments, XmlWriter results)
	{
		CheckArguments(input, results);
		Transform(input, arguments, results, XsltConfigSection.CreateDefaultResolver());
	}

	public void Transform(XmlReader input, XsltArgumentList arguments, TextWriter results)
	{
		CheckArguments(input, results);
		using XmlWriter xmlWriter = XmlWriter.Create(results, OutputSettings);
		Transform(input, arguments, xmlWriter, XsltConfigSection.CreateDefaultResolver());
		xmlWriter.Close();
	}

	public void Transform(XmlReader input, XsltArgumentList arguments, Stream results)
	{
		CheckArguments(input, results);
		using XmlWriter xmlWriter = XmlWriter.Create(results, OutputSettings);
		Transform(input, arguments, xmlWriter, XsltConfigSection.CreateDefaultResolver());
		xmlWriter.Close();
	}

	public void Transform(string inputUri, XmlWriter results)
	{
		CheckArguments(inputUri, results);
		using XmlReader input = XmlReader.Create(inputUri, ReaderSettings);
		Transform(input, null, results, XsltConfigSection.CreateDefaultResolver());
	}

	public void Transform(string inputUri, XsltArgumentList arguments, XmlWriter results)
	{
		CheckArguments(inputUri, results);
		using XmlReader input = XmlReader.Create(inputUri, ReaderSettings);
		Transform(input, arguments, results, XsltConfigSection.CreateDefaultResolver());
	}

	public void Transform(string inputUri, XsltArgumentList arguments, TextWriter results)
	{
		CheckArguments(inputUri, results);
		using XmlReader input = XmlReader.Create(inputUri, ReaderSettings);
		using XmlWriter xmlWriter = XmlWriter.Create(results, OutputSettings);
		Transform(input, arguments, xmlWriter, XsltConfigSection.CreateDefaultResolver());
		xmlWriter.Close();
	}

	public void Transform(string inputUri, XsltArgumentList arguments, Stream results)
	{
		CheckArguments(inputUri, results);
		using XmlReader input = XmlReader.Create(inputUri, ReaderSettings);
		using XmlWriter xmlWriter = XmlWriter.Create(results, OutputSettings);
		Transform(input, arguments, xmlWriter, XsltConfigSection.CreateDefaultResolver());
		xmlWriter.Close();
	}

	public void Transform(string inputUri, string resultsFile)
	{
		if (inputUri == null)
		{
			throw new ArgumentNullException("inputUri");
		}
		if (resultsFile == null)
		{
			throw new ArgumentNullException("resultsFile");
		}
		using XmlReader input = XmlReader.Create(inputUri, ReaderSettings);
		using XmlWriter xmlWriter = XmlWriter.Create(resultsFile, OutputSettings);
		Transform(input, null, xmlWriter, XsltConfigSection.CreateDefaultResolver());
		xmlWriter.Close();
	}

	public void Transform(XmlReader input, XsltArgumentList arguments, XmlWriter results, XmlResolver documentResolver)
	{
		CheckArguments(input, results);
		CheckCommand();
		command.Execute(input, documentResolver, arguments, results);
	}

	public void Transform(IXPathNavigable input, XsltArgumentList arguments, XmlWriter results, XmlResolver documentResolver)
	{
		CheckArguments(input, results);
		CheckCommand();
		command.Execute(input.CreateNavigator(), documentResolver, arguments, results);
	}

	private static void CheckArguments(object input, object results)
	{
		if (input == null)
		{
			throw new ArgumentNullException("input");
		}
		if (results == null)
		{
			throw new ArgumentNullException("results");
		}
	}

	private static void CheckArguments(string inputUri, object results)
	{
		if (inputUri == null)
		{
			throw new ArgumentNullException("inputUri");
		}
		if (results == null)
		{
			throw new ArgumentNullException("results");
		}
	}

	private void CheckCommand()
	{
		if (command == null)
		{
			throw new InvalidOperationException(Res.GetString("No stylesheet was loaded."));
		}
	}

	private QilExpression TestCompile(object stylesheet, XsltSettings settings, XmlResolver stylesheetResolver)
	{
		Reset();
		CompileXsltToQil(stylesheet, settings, stylesheetResolver);
		return qil;
	}

	private void TestGenerate(XsltSettings settings)
	{
		CompileQilToMsil(settings);
	}

	private void Transform(string inputUri, XsltArgumentList arguments, XmlWriter results, XmlResolver documentResolver)
	{
		command.Execute(inputUri, documentResolver, arguments, results);
	}

	internal static void PrintQil(object qil, XmlWriter xw, bool printComments, bool printTypes, bool printLineInfo)
	{
		QilExpression node = (QilExpression)qil;
		QilXmlWriter.Options options = QilXmlWriter.Options.None;
		if (printComments)
		{
			options |= QilXmlWriter.Options.Annotations;
		}
		if (printTypes)
		{
			options |= QilXmlWriter.Options.TypeInfo;
		}
		if (printLineInfo)
		{
			options |= QilXmlWriter.Options.LineInfo;
		}
		new QilXmlWriter(xw, options).ToXml(node);
		xw.Flush();
	}
}

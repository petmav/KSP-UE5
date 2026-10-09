using System.Collections;
using System.Configuration;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Reflection.Emit;
using System.Security;
using System.Security.Permissions;
using System.Security.Policy;
using System.Text;
using System.Threading;
using System.Xml.Serialization.Configuration;
using Microsoft.Win32;

namespace System.Xml.Serialization;

internal class TempAssembly
{
	internal class TempMethod
	{
		internal MethodInfo writeMethod;

		internal MethodInfo readMethod;

		internal string name;

		internal string ns;

		internal bool isSoap;

		internal string methodKey;
	}

	internal sealed class TempMethodDictionary : DictionaryBase
	{
		internal TempMethod this[string key] => (TempMethod)base.Dictionary[key];

		internal void Add(string key, TempMethod value)
		{
			base.Dictionary.Add(key, value);
		}
	}

	internal const string GeneratedAssemblyNamespace = "Microsoft.Xml.Serialization.GeneratedAssembly";

	private Assembly assembly;

	private bool pregeneratedAssmbly;

	private XmlSerializerImplementation contract;

	private Hashtable writerMethods;

	private Hashtable readerMethods;

	private TempMethodDictionary methods;

	private static object[] emptyObjectArray = new object[0];

	private Hashtable assemblies = new Hashtable();

	private static volatile FileIOPermission fileIOPermission;

	internal static bool UseLegacySerializerGeneration
	{
		get
		{
			if (AppSettings.UseLegacySerializerGeneration.HasValue)
			{
				return AppSettings.UseLegacySerializerGeneration.Value;
			}
			if (ConfigurationManager.GetSection(ConfigurationStrings.XmlSerializerSectionPath) is XmlSerializerSection xmlSerializerSection)
			{
				return xmlSerializerSection.UseLegacySerializerGeneration;
			}
			return false;
		}
	}

	internal XmlSerializerImplementation Contract
	{
		get
		{
			if (contract == null)
			{
				contract = (XmlSerializerImplementation)Activator.CreateInstance(GetTypeFromAssembly(assembly, "XmlSerializerContract"));
			}
			return contract;
		}
	}

	internal static FileIOPermission FileIOPermission
	{
		get
		{
			if (fileIOPermission == null)
			{
				fileIOPermission = new FileIOPermission(PermissionState.Unrestricted);
			}
			return fileIOPermission;
		}
	}

	internal bool NeedAssembyResolve
	{
		get
		{
			if (assemblies != null)
			{
				return assemblies.Count > 0;
			}
			return false;
		}
	}

	private TempAssembly()
	{
	}

	internal TempAssembly(XmlMapping[] xmlMappings, Type[] types, string defaultNamespace, string location, Evidence evidence)
	{
		bool flag = false;
		for (int i = 0; i < xmlMappings.Length; i++)
		{
			xmlMappings[i].CheckShallow();
			if (xmlMappings[i].IsSoap)
			{
				flag = true;
			}
		}
		bool flag2 = false;
		if (!flag && !UseLegacySerializerGeneration)
		{
			try
			{
				assembly = GenerateRefEmitAssembly(xmlMappings, types, defaultNamespace, evidence);
			}
			catch (CodeGeneratorConversionException)
			{
				flag2 = true;
			}
		}
		else
		{
			flag2 = true;
		}
		if (flag2)
		{
			assembly = GenerateAssembly(xmlMappings, types, defaultNamespace, evidence, XmlSerializerCompilerParameters.Create(location), null, assemblies);
		}
		InitAssemblyMethods(xmlMappings);
	}

	internal TempAssembly(XmlMapping[] xmlMappings, Assembly assembly, XmlSerializerImplementation contract)
	{
		this.assembly = assembly;
		InitAssemblyMethods(xmlMappings);
		this.contract = contract;
		pregeneratedAssmbly = true;
	}

	internal TempAssembly(XmlSerializerImplementation contract)
	{
		this.contract = contract;
		pregeneratedAssmbly = true;
	}

	internal void InitAssemblyMethods(XmlMapping[] xmlMappings)
	{
		methods = new TempMethodDictionary();
		for (int i = 0; i < xmlMappings.Length; i++)
		{
			TempMethod tempMethod = new TempMethod();
			tempMethod.isSoap = xmlMappings[i].IsSoap;
			tempMethod.methodKey = xmlMappings[i].Key;
			if (xmlMappings[i] is XmlTypeMapping xmlTypeMapping)
			{
				tempMethod.name = xmlTypeMapping.ElementName;
				tempMethod.ns = xmlTypeMapping.Namespace;
			}
			methods.Add(xmlMappings[i].Key, tempMethod);
		}
	}

	internal static Assembly LoadGeneratedAssembly(Type type, string defaultNamespace, out XmlSerializerImplementation contract)
	{
		Assembly assembly = null;
		contract = null;
		string text = null;
		if (Microsoft.Win32.UnsafeNativeMethods.IsPackagedProcess.Value)
		{
			return null;
		}
		bool enabled = DiagnosticsSwitches.PregenEventLog.Enabled;
		object[] customAttributes = type.GetCustomAttributes(typeof(XmlSerializerAssemblyAttribute), inherit: false);
		if (customAttributes.Length == 0)
		{
			AssemblyName name = GetName(type.Assembly, copyName: true);
			text = (name.Name = Compiler.GetTempAssemblyName(name, defaultNamespace));
			name.CodeBase = null;
			name.CultureInfo = CultureInfo.InvariantCulture;
			try
			{
				assembly = Assembly.Load(name);
			}
			catch (Exception ex)
			{
				if (ex is ThreadAbortException || ex is StackOverflowException || ex is OutOfMemoryException)
				{
					throw;
				}
				if (enabled)
				{
					Log(ex.Message, EventLogEntryType.Information);
				}
				byte[] publicKeyToken = name.GetPublicKeyToken();
				if (publicKeyToken != null && publicKeyToken.Length != 0)
				{
					return null;
				}
				assembly = Assembly.LoadWithPartialName(text, null);
			}
			if (assembly == null)
			{
				if (enabled)
				{
					Log(Res.GetString("Could not load file or assembly '{0}' or one of its dependencies. The system cannot find the file specified.", text), EventLogEntryType.Information);
				}
				return null;
			}
			if (!IsSerializerVersionMatch(assembly, type, defaultNamespace, null))
			{
				if (enabled)
				{
					Log(Res.GetString("Pre-generated serializer '{0}' has expired. You need to re-generate serializer for '{1}'.", text, type.FullName), EventLogEntryType.Error);
				}
				return null;
			}
		}
		else
		{
			XmlSerializerAssemblyAttribute xmlSerializerAssemblyAttribute = (XmlSerializerAssemblyAttribute)customAttributes[0];
			if (xmlSerializerAssemblyAttribute.AssemblyName != null && xmlSerializerAssemblyAttribute.CodeBase != null)
			{
				throw new InvalidOperationException(Res.GetString("Invalid XmlSerializerAssemblyAttribute usage. Please use {0} property or {1} property.", "AssemblyName", "CodeBase"));
			}
			if (xmlSerializerAssemblyAttribute.AssemblyName != null)
			{
				text = xmlSerializerAssemblyAttribute.AssemblyName;
				assembly = Assembly.LoadWithPartialName(text, null);
			}
			else if (xmlSerializerAssemblyAttribute.CodeBase != null && xmlSerializerAssemblyAttribute.CodeBase.Length > 0)
			{
				text = xmlSerializerAssemblyAttribute.CodeBase;
				assembly = Assembly.LoadFrom(text);
			}
			else
			{
				text = type.Assembly.FullName;
				assembly = type.Assembly;
			}
			if (assembly == null)
			{
				throw new FileNotFoundException(null, text);
			}
		}
		Type typeFromAssembly = GetTypeFromAssembly(assembly, "XmlSerializerContract");
		contract = (XmlSerializerImplementation)Activator.CreateInstance(typeFromAssembly);
		if (contract.CanSerialize(type))
		{
			return assembly;
		}
		if (enabled)
		{
			Log(Res.GetString("Pre-generated serializer '{0}' has expired. You need to re-generate serializer for '{1}'.", text, type.FullName), EventLogEntryType.Error);
		}
		return null;
	}

	private static void Log(string message, EventLogEntryType type)
	{
		new EventLogPermission(PermissionState.Unrestricted).Assert();
		EventLog.WriteEntry("XmlSerializer", message, type);
	}

	private static AssemblyName GetName(Assembly assembly, bool copyName)
	{
		PermissionSet permissionSet = new PermissionSet(PermissionState.None);
		permissionSet.AddPermission(new FileIOPermission(PermissionState.Unrestricted));
		permissionSet.Assert();
		return assembly.GetName(copyName);
	}

	private static bool IsSerializerVersionMatch(Assembly serializer, Type type, string defaultNamespace, string location)
	{
		if (serializer == null)
		{
			return false;
		}
		object[] customAttributes = serializer.GetCustomAttributes(typeof(XmlSerializerVersionAttribute), inherit: false);
		if (customAttributes.Length != 1)
		{
			return false;
		}
		XmlSerializerVersionAttribute xmlSerializerVersionAttribute = (XmlSerializerVersionAttribute)customAttributes[0];
		if (xmlSerializerVersionAttribute.ParentAssemblyId == GenerateAssemblyId(type) && xmlSerializerVersionAttribute.Namespace == defaultNamespace)
		{
			return true;
		}
		return false;
	}

	private static string GenerateAssemblyId(Type type)
	{
		Module[] modules = type.Assembly.GetModules();
		ArrayList arrayList = new ArrayList();
		for (int i = 0; i < modules.Length; i++)
		{
			arrayList.Add(modules[i].ModuleVersionId.ToString());
		}
		arrayList.Sort();
		StringBuilder stringBuilder = new StringBuilder();
		for (int j = 0; j < arrayList.Count; j++)
		{
			stringBuilder.Append(arrayList[j].ToString());
			stringBuilder.Append(",");
		}
		return stringBuilder.ToString();
	}

	internal static Assembly GenerateAssembly(XmlMapping[] xmlMappings, Type[] types, string defaultNamespace, Evidence evidence, XmlSerializerCompilerParameters parameters, Assembly assembly, Hashtable assemblies)
	{
		FileIOPermission.Assert();
		Compiler compiler = new Compiler();
		try
		{
			Hashtable hashtable = new Hashtable();
			foreach (XmlMapping xmlMapping in xmlMappings)
			{
				hashtable[xmlMapping.Scope] = xmlMapping;
			}
			TypeScope[] array = new TypeScope[hashtable.Keys.Count];
			hashtable.Keys.CopyTo(array, 0);
			assemblies.Clear();
			Hashtable types2 = new Hashtable();
			TypeScope[] array2 = array;
			for (int i = 0; i < array2.Length; i++)
			{
				foreach (Type type3 in array2[i].Types)
				{
					compiler.AddImport(type3, types2);
					Assembly assembly2 = type3.Assembly;
					string fullName = assembly2.FullName;
					if (assemblies[fullName] == null && !assembly2.GlobalAssemblyCache)
					{
						assemblies[fullName] = assembly2;
					}
				}
			}
			for (int j = 0; j < types.Length; j++)
			{
				compiler.AddImport(types[j], types2);
			}
			compiler.AddImport(typeof(object).Assembly);
			compiler.AddImport(typeof(XmlSerializer).Assembly);
			IndentedWriter indentedWriter = new IndentedWriter(compiler.Source, compact: false);
			indentedWriter.WriteLine("#if _DYNAMIC_XMLSERIALIZER_COMPILATION");
			indentedWriter.WriteLine("[assembly:System.Security.AllowPartiallyTrustedCallers()]");
			indentedWriter.WriteLine("[assembly:System.Security.SecurityTransparent()]");
			indentedWriter.WriteLine("[assembly:System.Security.SecurityRules(System.Security.SecurityRuleSet.Level1)]");
			indentedWriter.WriteLine("#endif");
			if (types != null && types.Length != 0 && types[0] != null)
			{
				indentedWriter.WriteLine("[assembly:System.Reflection.AssemblyVersionAttribute(\"" + types[0].Assembly.GetName().Version.ToString() + "\")]");
			}
			if (assembly != null && types.Length != 0)
			{
				for (int k = 0; k < types.Length; k++)
				{
					Type type2 = types[k];
					if (!(type2 == null) && DynamicAssemblies.IsTypeDynamic(type2))
					{
						throw new InvalidOperationException(Res.GetString("Cannot pre-generate serialization assembly for type '{0}'. Pre-generation of serialization assemblies is not supported for dynamic types. Save the assembly and load it from disk to use it with XmlSerialization.", types[k].FullName));
					}
				}
				indentedWriter.Write("[assembly:");
				indentedWriter.Write(typeof(XmlSerializerVersionAttribute).FullName);
				indentedWriter.Write("(");
				indentedWriter.Write("ParentAssemblyId=");
				ReflectionAwareCodeGen.WriteQuotedCSharpString(indentedWriter, GenerateAssemblyId(types[0]));
				indentedWriter.Write(", Version=");
				ReflectionAwareCodeGen.WriteQuotedCSharpString(indentedWriter, "4.0.0.0");
				if (defaultNamespace != null)
				{
					indentedWriter.Write(", Namespace=");
					ReflectionAwareCodeGen.WriteQuotedCSharpString(indentedWriter, defaultNamespace);
				}
				indentedWriter.WriteLine(")]");
			}
			CodeIdentifiers codeIdentifiers = new CodeIdentifiers();
			codeIdentifiers.AddUnique("XmlSerializationWriter", "XmlSerializationWriter");
			codeIdentifiers.AddUnique("XmlSerializationReader", "XmlSerializationReader");
			string text = null;
			if (types != null && types.Length == 1 && types[0] != null)
			{
				text = CodeIdentifier.MakeValid(types[0].Name);
				if (types[0].IsArray)
				{
					text += "Array";
				}
			}
			indentedWriter.WriteLine("namespace Microsoft.Xml.Serialization.GeneratedAssembly {");
			indentedWriter.Indent++;
			indentedWriter.WriteLine();
			string text2 = "XmlSerializationWriter" + text;
			text2 = codeIdentifiers.AddUnique(text2, text2);
			XmlSerializationWriterCodeGen xmlSerializationWriterCodeGen = new XmlSerializationWriterCodeGen(indentedWriter, array, "public", text2);
			xmlSerializationWriterCodeGen.GenerateBegin();
			string[] array3 = new string[xmlMappings.Length];
			for (int l = 0; l < xmlMappings.Length; l++)
			{
				array3[l] = xmlSerializationWriterCodeGen.GenerateElement(xmlMappings[l]);
			}
			xmlSerializationWriterCodeGen.GenerateEnd();
			indentedWriter.WriteLine();
			string text3 = "XmlSerializationReader" + text;
			text3 = codeIdentifiers.AddUnique(text3, text3);
			XmlSerializationReaderCodeGen xmlSerializationReaderCodeGen = new XmlSerializationReaderCodeGen(indentedWriter, array, "public", text3);
			xmlSerializationReaderCodeGen.GenerateBegin();
			string[] array4 = new string[xmlMappings.Length];
			for (int m = 0; m < xmlMappings.Length; m++)
			{
				array4[m] = xmlSerializationReaderCodeGen.GenerateElement(xmlMappings[m]);
			}
			xmlSerializationReaderCodeGen.GenerateEnd(array4, xmlMappings, types);
			string baseSerializer = xmlSerializationReaderCodeGen.GenerateBaseSerializer("XmlSerializer1", text3, text2, codeIdentifiers);
			Hashtable hashtable2 = new Hashtable();
			for (int n = 0; n < xmlMappings.Length; n++)
			{
				if (hashtable2[xmlMappings[n].Key] == null)
				{
					hashtable2[xmlMappings[n].Key] = xmlSerializationReaderCodeGen.GenerateTypedSerializer(array4[n], array3[n], xmlMappings[n], codeIdentifiers, baseSerializer, text3, text2);
				}
			}
			xmlSerializationReaderCodeGen.GenerateSerializerContract("XmlSerializerContract", xmlMappings, types, text3, array4, text2, array3, hashtable2);
			indentedWriter.Indent--;
			indentedWriter.WriteLine("}");
			return compiler.Compile(assembly, defaultNamespace, parameters, evidence);
		}
		finally
		{
			compiler.Close();
		}
	}

	internal static Assembly GenerateRefEmitAssembly(XmlMapping[] xmlMappings, Type[] types, string defaultNamespace, Evidence evidence)
	{
		Hashtable hashtable = new Hashtable();
		foreach (XmlMapping xmlMapping in xmlMappings)
		{
			hashtable[xmlMapping.Scope] = xmlMapping;
		}
		TypeScope[] array = new TypeScope[hashtable.Keys.Count];
		hashtable.Keys.CopyTo(array, 0);
		string text = "Microsoft.GeneratedCode";
		AssemblyBuilder assemblyBuilder = CodeGenerator.CreateAssemblyBuilder(AppDomain.CurrentDomain, text);
		ConstructorInfo constructor = typeof(SecurityTransparentAttribute).GetConstructor(CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		assemblyBuilder.SetCustomAttribute(new CustomAttributeBuilder(constructor, new object[0]));
		ConstructorInfo constructor2 = typeof(AllowPartiallyTrustedCallersAttribute).GetConstructor(CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		assemblyBuilder.SetCustomAttribute(new CustomAttributeBuilder(constructor2, new object[0]));
		ConstructorInfo constructor3 = typeof(SecurityRulesAttribute).GetConstructor(CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(SecurityRuleSet) }, null);
		assemblyBuilder.SetCustomAttribute(new CustomAttributeBuilder(constructor3, new object[1] { SecurityRuleSet.Level1 }));
		if (types != null && types.Length != 0 && types[0] != null)
		{
			ConstructorInfo constructor4 = typeof(AssemblyVersionAttribute).GetConstructor(CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(string) }, null);
			FileIOPermission.Assert();
			string text2 = types[0].Assembly.GetName().Version.ToString();
			CodeAccessPermission.RevertAssert();
			assemblyBuilder.SetCustomAttribute(new CustomAttributeBuilder(constructor4, new object[1] { text2 }));
		}
		CodeIdentifiers codeIdentifiers = new CodeIdentifiers();
		codeIdentifiers.AddUnique("XmlSerializationWriter", "XmlSerializationWriter");
		codeIdentifiers.AddUnique("XmlSerializationReader", "XmlSerializationReader");
		string text3 = null;
		if (types != null && types.Length == 1 && types[0] != null)
		{
			text3 = CodeIdentifier.MakeValid(types[0].Name);
			if (types[0].IsArray)
			{
				text3 += "Array";
			}
		}
		ModuleBuilder moduleBuilder = CodeGenerator.CreateModuleBuilder(assemblyBuilder, text);
		string text4 = "XmlSerializationWriter" + text3;
		text4 = codeIdentifiers.AddUnique(text4, text4);
		XmlSerializationWriterILGen xmlSerializationWriterILGen = new XmlSerializationWriterILGen(array, "public", text4);
		xmlSerializationWriterILGen.ModuleBuilder = moduleBuilder;
		xmlSerializationWriterILGen.GenerateBegin();
		string[] array2 = new string[xmlMappings.Length];
		for (int j = 0; j < xmlMappings.Length; j++)
		{
			array2[j] = xmlSerializationWriterILGen.GenerateElement(xmlMappings[j]);
		}
		Type type = xmlSerializationWriterILGen.GenerateEnd();
		string text5 = "XmlSerializationReader" + text3;
		text5 = codeIdentifiers.AddUnique(text5, text5);
		XmlSerializationReaderILGen xmlSerializationReaderILGen = new XmlSerializationReaderILGen(array, "public", text5);
		xmlSerializationReaderILGen.ModuleBuilder = moduleBuilder;
		xmlSerializationReaderILGen.CreatedTypes.Add(type.Name, type);
		xmlSerializationReaderILGen.GenerateBegin();
		string[] array3 = new string[xmlMappings.Length];
		for (int k = 0; k < xmlMappings.Length; k++)
		{
			array3[k] = xmlSerializationReaderILGen.GenerateElement(xmlMappings[k]);
		}
		xmlSerializationReaderILGen.GenerateEnd(array3, xmlMappings, types);
		string baseSerializer = xmlSerializationReaderILGen.GenerateBaseSerializer("XmlSerializer1", text5, text4, codeIdentifiers);
		Hashtable hashtable2 = new Hashtable();
		for (int l = 0; l < xmlMappings.Length; l++)
		{
			if (hashtable2[xmlMappings[l].Key] == null)
			{
				hashtable2[xmlMappings[l].Key] = xmlSerializationReaderILGen.GenerateTypedSerializer(array3[l], array2[l], xmlMappings[l], codeIdentifiers, baseSerializer, text5, text4);
			}
		}
		xmlSerializationReaderILGen.GenerateSerializerContract("XmlSerializerContract", xmlMappings, types, text5, array3, text4, array2, hashtable2);
		if (DiagnosticsSwitches.KeepTempFiles.Enabled)
		{
			FileIOPermission.Assert();
			assemblyBuilder.Save(text + ".dll");
		}
		return type.Assembly;
	}

	private static MethodInfo GetMethodFromType(Type type, string methodName, Assembly assembly)
	{
		MethodInfo method = type.GetMethod(methodName);
		if (method != null)
		{
			return method;
		}
		MissingMethodException ex = new MissingMethodException(type.FullName, methodName);
		if (assembly != null)
		{
			throw new InvalidOperationException(Res.GetString("Pre-generated assembly '{0}' CodeBase='{1}' has expired.", assembly.FullName, assembly.CodeBase), ex);
		}
		throw ex;
	}

	internal static Type GetTypeFromAssembly(Assembly assembly, string typeName)
	{
		typeName = "Microsoft.Xml.Serialization.GeneratedAssembly." + typeName;
		Type type = assembly.GetType(typeName);
		if (type == null)
		{
			throw new InvalidOperationException(Res.GetString("Invalid serialization assembly: Required type {0} cannot be found in the generated assembly '{1}'.", typeName, assembly.FullName));
		}
		return type;
	}

	internal bool CanRead(XmlMapping mapping, XmlReader xmlReader)
	{
		if (mapping == null)
		{
			return false;
		}
		if (mapping.Accessor.Any)
		{
			return true;
		}
		TempMethod tempMethod = methods[mapping.Key];
		return xmlReader.IsStartElement(tempMethod.name, tempMethod.ns);
	}

	private string ValidateEncodingStyle(string encodingStyle, string methodKey)
	{
		if (encodingStyle != null && encodingStyle.Length > 0)
		{
			if (!methods[methodKey].isSoap)
			{
				throw new InvalidOperationException(Res.GetString("The encoding style '{0}' is not valid for this call because this XmlSerializer instance does not support encoding. Use the SoapReflectionImporter to initialize an XmlSerializer that supports encoding.", encodingStyle));
			}
			if (encodingStyle != "http://schemas.xmlsoap.org/soap/encoding/" && encodingStyle != "http://www.w3.org/2003/05/soap-encoding")
			{
				throw new InvalidOperationException(Res.GetString("The encoding style '{0}' is not valid for this call. Valid values are '{1}' for SOAP 1.1 encoding or '{2}' for SOAP 1.2 encoding.", encodingStyle, "http://schemas.xmlsoap.org/soap/encoding/", "http://www.w3.org/2003/05/soap-encoding"));
			}
		}
		else if (methods[methodKey].isSoap)
		{
			encodingStyle = "http://schemas.xmlsoap.org/soap/encoding/";
		}
		return encodingStyle;
	}

	internal object InvokeReader(XmlMapping mapping, XmlReader xmlReader, XmlDeserializationEvents events, string encodingStyle)
	{
		XmlSerializationReader xmlSerializationReader = null;
		try
		{
			encodingStyle = ValidateEncodingStyle(encodingStyle, mapping.Key);
			xmlSerializationReader = Contract.Reader;
			xmlSerializationReader.Init(xmlReader, events, encodingStyle, this);
			if (methods[mapping.Key].readMethod == null)
			{
				if (readerMethods == null)
				{
					readerMethods = Contract.ReadMethods;
				}
				string text = (string)readerMethods[mapping.Key];
				if (text == null)
				{
					throw new InvalidOperationException(Res.GetString("Type '{0}' is not serializable.", mapping.Accessor.Name));
				}
				methods[mapping.Key].readMethod = GetMethodFromType(xmlSerializationReader.GetType(), text, pregeneratedAssmbly ? assembly : null);
			}
			return methods[mapping.Key].readMethod.Invoke(xmlSerializationReader, emptyObjectArray);
		}
		catch (SecurityException innerException)
		{
			throw new InvalidOperationException(Res.GetString("One or more assemblies referenced by the XmlSerializer cannot be called from partially trusted code."), innerException);
		}
		finally
		{
			xmlSerializationReader?.Dispose();
		}
	}

	internal void InvokeWriter(XmlMapping mapping, XmlWriter xmlWriter, object o, XmlSerializerNamespaces namespaces, string encodingStyle, string id)
	{
		XmlSerializationWriter xmlSerializationWriter = null;
		try
		{
			encodingStyle = ValidateEncodingStyle(encodingStyle, mapping.Key);
			xmlSerializationWriter = Contract.Writer;
			xmlSerializationWriter.Init(xmlWriter, namespaces, encodingStyle, id, this);
			if (methods[mapping.Key].writeMethod == null)
			{
				if (writerMethods == null)
				{
					writerMethods = Contract.WriteMethods;
				}
				string text = (string)writerMethods[mapping.Key];
				if (text == null)
				{
					throw new InvalidOperationException(Res.GetString("Type '{0}' is not serializable.", mapping.Accessor.Name));
				}
				methods[mapping.Key].writeMethod = GetMethodFromType(xmlSerializationWriter.GetType(), text, pregeneratedAssmbly ? assembly : null);
			}
			methods[mapping.Key].writeMethod.Invoke(xmlSerializationWriter, new object[1] { o });
		}
		catch (SecurityException innerException)
		{
			throw new InvalidOperationException(Res.GetString("One or more assemblies referenced by the XmlSerializer cannot be called from partially trusted code."), innerException);
		}
		finally
		{
			xmlSerializationWriter?.Dispose();
		}
	}

	internal Assembly GetReferencedAssembly(string name)
	{
		if (assemblies == null || name == null)
		{
			return null;
		}
		return (Assembly)assemblies[name];
	}
}

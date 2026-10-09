using System.CodeDom.Compiler;
using System.Collections;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Security;
using System.Security.Permissions;
using System.Security.Policy;
using System.Security.Principal;
using System.Threading;
using Microsoft.CSharp;

namespace System.Xml.Serialization;

internal class Compiler
{
	private bool debugEnabled = DiagnosticsSwitches.KeepTempFiles.Enabled;

	private Hashtable imports = new Hashtable();

	private StringWriter writer = new StringWriter(CultureInfo.InvariantCulture);

	protected string[] Imports
	{
		get
		{
			string[] array = new string[imports.Values.Count];
			imports.Values.CopyTo(array, 0);
			return array;
		}
	}

	internal TextWriter Source => writer;

	internal void AddImport(Type type, Hashtable types)
	{
		if (type == null || TypeScope.IsKnownType(type) || types[type] != null)
		{
			return;
		}
		types[type] = type;
		Type baseType = type.BaseType;
		if (baseType != null)
		{
			AddImport(baseType, types);
		}
		Type declaringType = type.DeclaringType;
		if (declaringType != null)
		{
			AddImport(declaringType, types);
		}
		Type[] interfaces = type.GetInterfaces();
		foreach (Type type2 in interfaces)
		{
			AddImport(type2, types);
		}
		ConstructorInfo[] constructors = type.GetConstructors();
		for (int j = 0; j < constructors.Length; j++)
		{
			ParameterInfo[] parameters = constructors[j].GetParameters();
			for (int k = 0; k < parameters.Length; k++)
			{
				AddImport(parameters[k].ParameterType, types);
			}
		}
		if (type.IsGenericType)
		{
			Type[] genericArguments = type.GetGenericArguments();
			for (int l = 0; l < genericArguments.Length; l++)
			{
				AddImport(genericArguments[l], types);
			}
		}
		TempAssembly.FileIOPermission.Assert();
		Assembly assembly = type.Module.Assembly;
		if (DynamicAssemblies.IsTypeDynamic(type))
		{
			DynamicAssemblies.Add(assembly);
			return;
		}
		object[] customAttributes = type.GetCustomAttributes(typeof(TypeForwardedFromAttribute), inherit: false);
		if (customAttributes.Length != 0)
		{
			Assembly assembly2 = Assembly.Load((customAttributes[0] as TypeForwardedFromAttribute).AssemblyFullName);
			imports[assembly2] = assembly2.Location;
		}
		imports[assembly] = assembly.Location;
	}

	internal void AddImport(Assembly assembly)
	{
		TempAssembly.FileIOPermission.Assert();
		imports[assembly] = assembly.Location;
	}

	internal void Close()
	{
	}

	internal static string GetTempAssemblyPath(string baseDir, Assembly assembly, string defaultNamespace)
	{
		if (assembly.IsDynamic)
		{
			throw new InvalidOperationException(Res.GetString("Cannot pre-generate serialization assembly. Pre-generation of serialization assemblies is not supported for dynamic assemblies. Save the assembly and load it from disk to use it with XmlSerialization."));
		}
		PermissionSet permissionSet = new PermissionSet(PermissionState.None);
		permissionSet.AddPermission(new FileIOPermission(PermissionState.Unrestricted));
		permissionSet.AddPermission(new EnvironmentPermission(PermissionState.Unrestricted));
		permissionSet.Assert();
		try
		{
			if (baseDir != null && baseDir.Length > 0)
			{
				if (!Directory.Exists(baseDir))
				{
					throw new UnauthorizedAccessException(Res.GetString("Could not find directory to save XmlSerializer generated assembly: {0}.", baseDir));
				}
			}
			else
			{
				baseDir = Path.GetTempPath();
				if (!Directory.Exists(baseDir))
				{
					throw new UnauthorizedAccessException(Res.GetString("Could not find TEMP directory to save XmlSerializer generated assemblies."));
				}
			}
			baseDir = Path.Combine(baseDir, GetTempAssemblyName(assembly.GetName(), defaultNamespace));
		}
		finally
		{
			CodeAccessPermission.RevertAssert();
		}
		return baseDir + ".dll";
	}

	internal static string GetTempAssemblyName(AssemblyName parent, string ns)
	{
		return parent.Name + ".XmlSerializers" + ((ns == null || ns.Length == 0) ? "" : ("." + ns.GetHashCode()));
	}

	internal Assembly Compile(Assembly parent, string ns, XmlSerializerCompilerParameters xmlParameters, Evidence evidence)
	{
		CodeDomProvider codeDomProvider = new CSharpCodeProvider();
		CompilerParameters codeDomParameters = xmlParameters.CodeDomParameters;
		codeDomParameters.ReferencedAssemblies.AddRange(Imports);
		if (debugEnabled)
		{
			codeDomParameters.GenerateInMemory = false;
			codeDomParameters.IncludeDebugInformation = true;
			codeDomParameters.TempFiles.KeepFiles = true;
		}
		PermissionSet permissionSet = new PermissionSet(PermissionState.None);
		if (xmlParameters.IsNeedTempDirAccess)
		{
			permissionSet.AddPermission(TempAssembly.FileIOPermission);
		}
		permissionSet.AddPermission(new EnvironmentPermission(PermissionState.Unrestricted));
		permissionSet.AddPermission(new SecurityPermission(SecurityPermissionFlag.UnmanagedCode));
		permissionSet.AddPermission(new SecurityPermission(SecurityPermissionFlag.ControlEvidence));
		permissionSet.Assert();
		if (parent != null && (codeDomParameters.OutputAssembly == null || codeDomParameters.OutputAssembly.Length == 0))
		{
			string text = AssemblyNameFromOptions(codeDomParameters.CompilerOptions);
			if (text == null)
			{
				text = GetTempAssemblyPath(codeDomParameters.TempFiles.TempDir, parent, ns);
			}
			codeDomParameters.OutputAssembly = text;
		}
		if (codeDomParameters.CompilerOptions == null || codeDomParameters.CompilerOptions.Length == 0)
		{
			codeDomParameters.CompilerOptions = "/nostdlib";
		}
		else
		{
			codeDomParameters.CompilerOptions += " /nostdlib";
		}
		codeDomParameters.CompilerOptions += " /D:_DYNAMIC_XMLSERIALIZER_COMPILATION";
		codeDomParameters.Evidence = evidence;
		CompilerResults compilerResults = null;
		Assembly assembly = null;
		try
		{
			compilerResults = codeDomProvider.CompileAssemblyFromSource(codeDomParameters, writer.ToString());
			if (compilerResults.Errors.Count > 0)
			{
				StringWriter stringWriter = new StringWriter(CultureInfo.InvariantCulture);
				stringWriter.WriteLine(Res.GetString("Unable to generate a temporary class (result={0}).", compilerResults.NativeCompilerReturnValue.ToString(CultureInfo.InvariantCulture)));
				bool flag = false;
				foreach (CompilerError error in compilerResults.Errors)
				{
					error.FileName = "";
					if (!error.IsWarning || error.ErrorNumber == "CS1595")
					{
						flag = true;
						stringWriter.WriteLine(error.ToString());
					}
				}
				if (flag)
				{
					throw new InvalidOperationException(stringWriter.ToString());
				}
			}
			assembly = compilerResults.CompiledAssembly;
		}
		catch (UnauthorizedAccessException)
		{
			string currentUser = GetCurrentUser();
			if (currentUser == null || currentUser.Length == 0)
			{
				throw new UnauthorizedAccessException(Res.GetString("Access to the temp directory is denied.  The process under which XmlSerializer is running does not have sufficient permission to access the temp directory.  CodeDom will use the user account the process is using to do the compilation, so if the user doesn\ufffdt have access to system temp directory, you will not be able to compile.  Use Path.GetTempPath() API to find out the temp directory location."));
			}
			throw new UnauthorizedAccessException(Res.GetString("Access to the temp directory is denied.  Identity '{0}' under which XmlSerializer is running does not have sufficient permission to access the temp directory.  CodeDom will use the user account the process is using to do the compilation, so if the user doesn\ufffdt have access to system temp directory, you will not be able to compile.  Use Path.GetTempPath() API to find out the temp directory location.", currentUser));
		}
		catch (FileLoadException innerException)
		{
			throw new InvalidOperationException(Res.GetString("Cannot load dynamically generated serialization assembly. In some hosting environments assembly load functionality is restricted, consider using pre-generated serializer. Please see inner exception for more information."), innerException);
		}
		finally
		{
			CodeAccessPermission.RevertAssert();
		}
		if (assembly == null)
		{
			throw new InvalidOperationException(Res.GetString("Internal error."));
		}
		return assembly;
	}

	private static string AssemblyNameFromOptions(string options)
	{
		if (options == null || options.Length == 0)
		{
			return null;
		}
		string result = null;
		string[] array = options.ToLower(CultureInfo.InvariantCulture).Split(null);
		for (int i = 0; i < array.Length; i++)
		{
			string text = array[i].Trim();
			if (text.StartsWith("/out:", StringComparison.Ordinal))
			{
				result = text.Substring(5);
			}
		}
		return result;
	}

	internal static string GetCurrentUser()
	{
		try
		{
			WindowsIdentity current = WindowsIdentity.GetCurrent();
			if (current != null && current.Name != null)
			{
				return current.Name;
			}
		}
		catch (Exception ex)
		{
			if (ex is ThreadAbortException || ex is StackOverflowException || ex is OutOfMemoryException)
			{
				throw;
			}
		}
		return "";
	}
}

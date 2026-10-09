using System.Collections;
using System.Diagnostics;
using System.Diagnostics.SymbolStore;
using System.Reflection;
using System.Reflection.Emit;
using System.Security;
using System.Security.Permissions;
using System.Threading;
using System.Xml.Xsl.Runtime;

namespace System.Xml.Xsl.IlGen;

internal class XmlILModule
{
	public static readonly PermissionSet CreateModulePermissionSet;

	private static long AssemblyId;

	private static ModuleBuilder LREModule;

	private TypeBuilder typeBldr;

	private Hashtable methods;

	private Hashtable urlToSymWriter;

	private string modFile;

	private bool persistAsm;

	private bool useLRE;

	private bool emitSymbols;

	private static readonly Guid LanguageGuid;

	private static readonly Guid VendorGuid;

	private const string RuntimeName = "{urn:schemas-microsoft-com:xslt-debug}runtime";

	public bool EmitSymbols => emitSymbols;

	static XmlILModule()
	{
		LanguageGuid = new Guid(1177373246u, 45655, 19182, 151, 205, 89, 24, 199, 83, 23, 88);
		VendorGuid = new Guid(2571847108u, 59113, 4562, 144, 63, 0, 192, 79, 163, 2, 161);
		CreateModulePermissionSet = new PermissionSet(PermissionState.None);
		CreateModulePermissionSet.AddPermission(new ReflectionPermission(ReflectionPermissionFlag.MemberAccess));
		CreateModulePermissionSet.AddPermission(new SecurityPermission(SecurityPermissionFlag.UnmanagedCode | SecurityPermissionFlag.ControlEvidence));
		AssemblyId = 0L;
		AssemblyName name = CreateAssemblyName();
		AssemblyBuilder assemblyBuilder = AppDomain.CurrentDomain.DefineDynamicAssembly(name, AssemblyBuilderAccess.Run);
		try
		{
			CreateModulePermissionSet.Assert();
			assemblyBuilder.SetCustomAttribute(new CustomAttributeBuilder(XmlILConstructors.Transparent, new object[0]));
			LREModule = assemblyBuilder.DefineDynamicModule("System.Xml.Xsl.CompiledQuery", emitSymbolInfo: false);
		}
		finally
		{
			CodeAccessPermission.RevertAssert();
		}
	}

	public XmlILModule(TypeBuilder typeBldr)
	{
		this.typeBldr = typeBldr;
		emitSymbols = ((ModuleBuilder)this.typeBldr.Module).GetSymWriter() != null;
		useLRE = false;
		persistAsm = false;
		methods = new Hashtable();
		if (emitSymbols)
		{
			urlToSymWriter = new Hashtable();
		}
	}

	public XmlILModule(bool useLRE, bool emitSymbols)
	{
		this.useLRE = useLRE;
		this.emitSymbols = emitSymbols;
		persistAsm = false;
		methods = new Hashtable();
		if (!useLRE)
		{
			AssemblyName name = CreateAssemblyName();
			AssemblyBuilder assemblyBuilder = AppDomain.CurrentDomain.DefineDynamicAssembly(name, (!persistAsm) ? AssemblyBuilderAccess.Run : AssemblyBuilderAccess.RunAndSave);
			assemblyBuilder.SetCustomAttribute(new CustomAttributeBuilder(XmlILConstructors.Transparent, new object[0]));
			if (emitSymbols)
			{
				urlToSymWriter = new Hashtable();
				DebuggableAttribute.DebuggingModes debuggingModes = DebuggableAttribute.DebuggingModes.Default | DebuggableAttribute.DebuggingModes.DisableOptimizations | DebuggableAttribute.DebuggingModes.IgnoreSymbolStoreSequencePoints;
				assemblyBuilder.SetCustomAttribute(new CustomAttributeBuilder(XmlILConstructors.Debuggable, new object[1] { debuggingModes }));
			}
			typeBldr = ((!persistAsm) ? assemblyBuilder.DefineDynamicModule("System.Xml.Xsl.CompiledQuery", emitSymbols) : assemblyBuilder.DefineDynamicModule("System.Xml.Xsl.CompiledQuery", modFile + ".dll", emitSymbols)).DefineType("System.Xml.Xsl.CompiledQuery.Query", TypeAttributes.Public);
		}
	}

	public MethodInfo DefineMethod(string name, Type returnType, Type[] paramTypes, string[] paramNames, XmlILMethodAttributes xmlAttrs)
	{
		int num = 1;
		string text = name;
		bool flag = (xmlAttrs & XmlILMethodAttributes.Raw) != 0;
		while (methods[name] != null)
		{
			num++;
			name = text + " (" + num + ")";
		}
		if (!flag)
		{
			Type[] array = new Type[paramTypes.Length + 1];
			array[0] = typeof(XmlQueryRuntime);
			Array.Copy(paramTypes, 0, array, 1, paramTypes.Length);
			paramTypes = array;
		}
		MethodInfo methodInfo;
		if (!useLRE)
		{
			MethodBuilder methodBuilder = typeBldr.DefineMethod(name, MethodAttributes.Private | MethodAttributes.Static, returnType, paramTypes);
			if (emitSymbols && (xmlAttrs & XmlILMethodAttributes.NonUser) != 0)
			{
				methodBuilder.SetCustomAttribute(new CustomAttributeBuilder(XmlILConstructors.StepThrough, new object[0]));
				methodBuilder.SetCustomAttribute(new CustomAttributeBuilder(XmlILConstructors.NonUserCode, new object[0]));
			}
			if (!flag)
			{
				methodBuilder.DefineParameter(1, ParameterAttributes.None, "{urn:schemas-microsoft-com:xslt-debug}runtime");
			}
			for (int i = 0; i < paramNames.Length; i++)
			{
				if (paramNames[i] != null && paramNames[i].Length != 0)
				{
					methodBuilder.DefineParameter(i + (flag ? 1 : 2), ParameterAttributes.None, paramNames[i]);
				}
			}
			methodInfo = methodBuilder;
		}
		else
		{
			DynamicMethod dynamicMethod = new DynamicMethod(name, returnType, paramTypes, LREModule);
			dynamicMethod.InitLocals = true;
			if (!flag)
			{
				dynamicMethod.DefineParameter(1, ParameterAttributes.None, "{urn:schemas-microsoft-com:xslt-debug}runtime");
			}
			for (int j = 0; j < paramNames.Length; j++)
			{
				if (paramNames[j] != null && paramNames[j].Length != 0)
				{
					dynamicMethod.DefineParameter(j + (flag ? 1 : 2), ParameterAttributes.None, paramNames[j]);
				}
			}
			methodInfo = dynamicMethod;
		}
		methods[name] = methodInfo;
		return methodInfo;
	}

	public static ILGenerator DefineMethodBody(MethodBase methInfo)
	{
		DynamicMethod dynamicMethod = methInfo as DynamicMethod;
		if (dynamicMethod != null)
		{
			return dynamicMethod.GetILGenerator();
		}
		MethodBuilder methodBuilder = methInfo as MethodBuilder;
		if (methodBuilder != null)
		{
			return methodBuilder.GetILGenerator();
		}
		return ((ConstructorBuilder)methInfo).GetILGenerator();
	}

	public MethodInfo FindMethod(string name)
	{
		return (MethodInfo)methods[name];
	}

	public FieldInfo DefineInitializedData(string name, byte[] data)
	{
		return typeBldr.DefineInitializedData(name, data, FieldAttributes.Private | FieldAttributes.Static);
	}

	public FieldInfo DefineField(string fieldName, Type type)
	{
		return typeBldr.DefineField(fieldName, type, FieldAttributes.Private | FieldAttributes.Static);
	}

	public ConstructorInfo DefineTypeInitializer()
	{
		return typeBldr.DefineTypeInitializer();
	}

	public ISymbolDocumentWriter AddSourceDocument(string fileName)
	{
		ISymbolDocumentWriter symbolDocumentWriter = urlToSymWriter[fileName] as ISymbolDocumentWriter;
		if (symbolDocumentWriter == null)
		{
			symbolDocumentWriter = ((ModuleBuilder)typeBldr.Module).DefineDocument(fileName, LanguageGuid, VendorGuid, Guid.Empty);
			urlToSymWriter.Add(fileName, symbolDocumentWriter);
		}
		return symbolDocumentWriter;
	}

	public void BakeMethods()
	{
		if (useLRE)
		{
			return;
		}
		Type type = typeBldr.CreateType();
		if (persistAsm)
		{
			((AssemblyBuilder)typeBldr.Module.Assembly).Save(modFile + ".dll");
		}
		Hashtable hashtable = new Hashtable(methods.Count);
		foreach (string key in methods.Keys)
		{
			hashtable[key] = type.GetMethod(key, BindingFlags.Static | BindingFlags.NonPublic);
		}
		methods = hashtable;
		typeBldr = null;
		urlToSymWriter = null;
	}

	public Delegate CreateDelegate(string name, Type typDelegate)
	{
		if (!useLRE)
		{
			return Delegate.CreateDelegate(typDelegate, (MethodInfo)methods[name]);
		}
		return ((DynamicMethod)methods[name]).CreateDelegate(typDelegate);
	}

	private static AssemblyName CreateAssemblyName()
	{
		Interlocked.Increment(ref AssemblyId);
		return new AssemblyName
		{
			Name = "System.Xml.Xsl.CompiledQuery." + AssemblyId
		};
	}
}

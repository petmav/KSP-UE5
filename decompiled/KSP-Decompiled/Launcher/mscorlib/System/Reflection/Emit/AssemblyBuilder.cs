using System.Collections.Generic;
using System.Resources;
using System.Runtime.InteropServices;
using System.Security;
using Unity;

namespace System.Reflection.Emit;

public class AssemblyBuilder : Assembly, _AssemblyBuilder
{
	public override PermissionSet PermissionSet
	{
		[SecurityCritical]
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public static AssemblyBuilder DefineDynamicAssembly(AssemblyName name, AssemblyBuilderAccess access)
	{
		throw new PlatformNotSupportedException();
	}

	public static AssemblyBuilder DefineDynamicAssembly(AssemblyName name, AssemblyBuilderAccess access, IEnumerable<CustomAttributeBuilder> assemblyAttributes)
	{
		throw new PlatformNotSupportedException();
	}

	public ModuleBuilder DefineDynamicModule(string name)
	{
		throw new PlatformNotSupportedException();
	}

	public ModuleBuilder GetDynamicModule(string name)
	{
		throw new PlatformNotSupportedException();
	}

	public void SetCustomAttribute(CustomAttributeBuilder customBuilder)
	{
		throw new PlatformNotSupportedException();
	}

	public void SetCustomAttribute(ConstructorInfo con, byte[] binaryAttribute)
	{
		throw new PlatformNotSupportedException();
	}

	public void AddResourceFile(string name, string fileName)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	public void AddResourceFile(string name, string fileName, ResourceAttributes attribute)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public ModuleBuilder DefineDynamicModule(string name, bool emitSymbolInfo)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecuritySafeCritical]
	public ModuleBuilder DefineDynamicModule(string name, string fileName)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecuritySafeCritical]
	public ModuleBuilder DefineDynamicModule(string name, string fileName, bool emitSymbolInfo)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public IResourceWriter DefineResource(string name, string description, string fileName)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public IResourceWriter DefineResource(string name, string description, string fileName, ResourceAttributes attribute)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void DefineUnmanagedResource(byte[] resource)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public void DefineUnmanagedResource(string resourceFileName)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	public void DefineVersionInfoResource()
	{
		ThrowStub.ThrowNotSupportedException();
	}

	public void DefineVersionInfoResource(string product, string productVersion, string company, string copyright, string trademark)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	public void Save(string assemblyFileName)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public void Save(string assemblyFileName, PortableExecutableKinds portableExecutableKind, ImageFileMachine imageFileMachine)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	public void SetEntryPoint(MethodInfo entryMethod)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	public void SetEntryPoint(MethodInfo entryMethod, PEFileKinds fileKind)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _AssemblyBuilder.GetIDsOfNames([In] ref Guid riid, IntPtr rgszNames, uint cNames, uint lcid, IntPtr rgDispId)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _AssemblyBuilder.GetTypeInfo(uint iTInfo, uint lcid, IntPtr ppTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _AssemblyBuilder.GetTypeInfoCount(out uint pcTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _AssemblyBuilder.Invoke(uint dispIdMember, [In] ref Guid riid, uint lcid, short wFlags, IntPtr pDispParams, IntPtr pVarResult, IntPtr pExcepInfo, IntPtr puArgErr)
	{
		ThrowStub.ThrowNotSupportedException();
	}
}

using System.Reflection;
using System.Reflection.Emit;
using System.Security;
using System.Security.Permissions;
using Unity;

namespace System.Runtime.InteropServices;

[ClassInterface(ClassInterfaceType.None)]
[ComVisible(true)]
[Guid("F1C3BF79-C3E4-11d3-88E7-00902754C43A")]
public sealed class TypeLibConverter : ITypeLibConverter
{
	public TypeLibConverter()
	{
		ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	[SecurityPermission(SecurityAction.Demand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	[return: MarshalAs(UnmanagedType.Interface)]
	public object ConvertAssemblyToTypeLib(Assembly assembly, string strTypeLibName, TypeLibExporterFlags flags, ITypeLibExporterNotifySink notifySink)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecuritySafeCritical]
	[SecurityPermission(SecurityAction.Demand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public AssemblyBuilder ConvertTypeLibToAssembly([MarshalAs(UnmanagedType.Interface)] object typeLib, string asmFileName, int flags, ITypeLibImporterNotifySink notifySink, byte[] publicKey, StrongNameKeyPair keyPair, bool unsafeInterfaces)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecuritySafeCritical]
	[SecurityPermission(SecurityAction.Demand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public AssemblyBuilder ConvertTypeLibToAssembly([MarshalAs(UnmanagedType.Interface)] object typeLib, string asmFileName, TypeLibImporterFlags flags, ITypeLibImporterNotifySink notifySink, byte[] publicKey, StrongNameKeyPair keyPair, string asmNamespace, Version asmVersion)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public bool GetPrimaryInteropAssembly(Guid g, int major, int minor, int lcid, out string asmName, out string asmCodeBase)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}

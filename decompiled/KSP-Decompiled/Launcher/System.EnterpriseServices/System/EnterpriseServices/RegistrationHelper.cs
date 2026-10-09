using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[Guid("89a86e7b-c229-4008-9baa-2f5c8411d7e0")]
public sealed class RegistrationHelper : MarshalByRefObject, IRegistrationHelper
{
	public RegistrationHelper()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void InstallAssembly(string assembly, ref string application, string partition, ref string tlb, InstallationFlags installFlags)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void InstallAssembly(string assembly, ref string application, ref string tlb, InstallationFlags installFlags)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void InstallAssemblyFromConfig([MarshalAs(UnmanagedType.IUnknown)] ref RegistrationConfig regConfig)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void UninstallAssembly(string assembly, string application)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void UninstallAssembly(string assembly, string application, string partition)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void UninstallAssemblyFromConfig([MarshalAs(UnmanagedType.IUnknown)] ref RegistrationConfig regConfig)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

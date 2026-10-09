using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[Obsolete("The RegistrationHelperTx class has been deprecated.")]
[Transaction(TransactionOption.RequiresNew)]
[Guid("c89ac250-e18a-4fc7-abd5-b8897b6a78a5")]
public sealed class RegistrationHelperTx : ServicedComponent
{
	public RegistrationHelperTx()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal override void Activate()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal override void Deactivate()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void InstallAssembly(string assembly, ref string application, string partition, ref string tlb, InstallationFlags installFlags, object sync)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void InstallAssembly(string assembly, ref string application, ref string tlb, InstallationFlags installFlags, object sync)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void InstallAssemblyFromConfig([MarshalAs(UnmanagedType.IUnknown)] ref RegistrationConfig regConfig, object sync)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool IsInTransaction()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void UninstallAssembly(string assembly, string application, object sync)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void UninstallAssembly(string assembly, string application, string partition, object sync)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void UninstallAssemblyFromConfig([MarshalAs(UnmanagedType.IUnknown)] ref RegistrationConfig regConfig, object sync)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

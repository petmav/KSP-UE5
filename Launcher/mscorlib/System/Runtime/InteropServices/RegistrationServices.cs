using System.Reflection;
using System.Security;
using Unity;

namespace System.Runtime.InteropServices;

[ClassInterface(ClassInterfaceType.None)]
[ComVisible(true)]
[Guid("475E398F-8AFA-43a7-A3BE-F4EF8D6787C9")]
public class RegistrationServices : IRegistrationServices
{
	public RegistrationServices()
	{
		ThrowStub.ThrowNotSupportedException();
	}

	public virtual Guid GetManagedCategoryGuid()
	{
		ThrowStub.ThrowNotSupportedException();
		return default(Guid);
	}

	[SecurityCritical]
	public virtual string GetProgIdForType(Type type)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecurityCritical]
	public virtual Type[] GetRegistrableTypesInAssembly(Assembly assembly)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecurityCritical]
	public virtual bool RegisterAssembly(Assembly assembly, AssemblyRegistrationFlags flags)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[SecurityCritical]
	public virtual void RegisterTypeForComClients(Type type, ref Guid g)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	[SecurityCritical]
	[ComVisible(false)]
	public virtual int RegisterTypeForComClients(Type type, RegistrationClassContext classContext, RegistrationConnectionType flags)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	[SecuritySafeCritical]
	public virtual bool TypeRepresentsComType(Type type)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[SecurityCritical]
	public virtual bool TypeRequiresRegistration(Type type)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[SecurityCritical]
	public virtual bool UnregisterAssembly(Assembly assembly)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[SecurityCritical]
	[ComVisible(false)]
	public virtual void UnregisterTypeForComClients(int cookie)
	{
		ThrowStub.ThrowNotSupportedException();
	}
}

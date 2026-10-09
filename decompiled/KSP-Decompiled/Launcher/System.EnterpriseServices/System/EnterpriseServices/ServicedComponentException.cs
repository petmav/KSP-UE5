using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[Serializable]
[ComVisible(false)]
public sealed class ServicedComponentException : SystemException
{
	public ServicedComponentException()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ServicedComponentException(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ServicedComponentException(string message, Exception innerException)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

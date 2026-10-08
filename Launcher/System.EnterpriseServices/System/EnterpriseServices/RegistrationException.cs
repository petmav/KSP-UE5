using Unity;

namespace System.EnterpriseServices;

[Serializable]
public sealed class RegistrationException : SystemException
{
	public RegistrationErrorInfo[] ErrorInfo
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public RegistrationException()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public RegistrationException(string msg)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public RegistrationException(string msg, Exception inner)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using Unity;

namespace System.EnterpriseServices;

[Serializable]
public sealed class RegistrationErrorInfo
{
	public int ErrorCode
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public string ErrorString
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string MajorRef
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string MinorRef
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string Name
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal RegistrationErrorInfo()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

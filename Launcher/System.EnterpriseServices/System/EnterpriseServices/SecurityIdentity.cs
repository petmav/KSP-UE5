using Unity;

namespace System.EnterpriseServices;

public sealed class SecurityIdentity
{
	public string AccountName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public AuthenticationOption AuthenticationLevel
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(AuthenticationOption);
		}
	}

	public int AuthenticationService
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public ImpersonationLevelOption ImpersonationLevel
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(ImpersonationLevelOption);
		}
	}

	internal SecurityIdentity()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using Unity;

namespace System.EnterpriseServices;

public sealed class SecurityCallContext
{
	public SecurityCallers Callers
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public static SecurityCallContext CurrentCall
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SecurityIdentity DirectCaller
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool IsSecurityEnabled
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public int MinAuthenticationLevel
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public int NumCallers
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public SecurityIdentity OriginalCaller
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal SecurityCallContext()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool IsCallerInRole(string role)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsUserInRole(string user, string role)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}

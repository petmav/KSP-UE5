using System.Transactions;
using Unity;

namespace System.EnterpriseServices;

public sealed class ContextUtil
{
	public static Guid ActivityId
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Guid);
		}
	}

	public static Guid ApplicationId
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Guid);
		}
	}

	public static Guid ApplicationInstanceId
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Guid);
		}
	}

	public static Guid ContextId
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Guid);
		}
	}

	public static bool DeactivateOnReturn
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public static bool IsInTransaction
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public static bool IsSecurityEnabled
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public static TransactionVote MyTransactionVote
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(TransactionVote);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public static Guid PartitionId
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Guid);
		}
	}

	public static Transaction SystemTransaction
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public static object Transaction
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public static Guid TransactionId
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Guid);
		}
	}

	internal ContextUtil()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void DisableCommit()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void EnableCommit()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static object GetNamedProperty(string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static bool IsCallerInRole(string role)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool IsDefaultContext()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static void SetAbort()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void SetComplete()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void SetNamedProperty(string name, object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

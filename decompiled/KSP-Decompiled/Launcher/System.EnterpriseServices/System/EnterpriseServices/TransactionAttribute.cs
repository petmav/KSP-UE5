using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[AttributeUsage(AttributeTargets.Class, Inherited = true)]
[ComVisible(false)]
public sealed class TransactionAttribute : Attribute
{
	public TransactionIsolationLevel Isolation
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(TransactionIsolationLevel);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int Timeout
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public TransactionOption Value
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(TransactionOption);
		}
	}

	public TransactionAttribute()
	{
	}

	public TransactionAttribute(TransactionOption val)
	{
	}
}

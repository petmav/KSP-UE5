using System.Collections;
using System.Runtime.CompilerServices;
using Unity;

namespace System.EnterpriseServices.CompensatingResourceManager;

public sealed class ClerkMonitor : IEnumerable
{
	public int Count
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public ClerkInfo this[string index]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ClerkMonitor()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	public ClerkInfo get_Item(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public IEnumerator GetEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Populate()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

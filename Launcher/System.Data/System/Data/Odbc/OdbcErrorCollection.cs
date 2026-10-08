using System.Collections;
using System.Runtime.CompilerServices;
using Unity;

namespace System.Data.Odbc;

[Serializable]
public sealed class OdbcErrorCollection : ICollection, IEnumerable
{
	public int Count
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public OdbcError this[int i]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal OdbcErrorCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	bool ICollection.get_IsSynchronized()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[SpecialName]
	object ICollection.get_SyncRoot()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void CopyTo(Array array, int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void CopyTo(OdbcError[] array, int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public IEnumerator GetEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

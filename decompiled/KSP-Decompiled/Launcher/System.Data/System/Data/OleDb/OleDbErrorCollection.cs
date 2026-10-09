using System.Collections;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using Unity;

namespace System.Data.OleDb;

[Serializable]
[ListBindable(false)]
public sealed class OleDbErrorCollection : ICollection, IEnumerable
{
	public int Count
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public OleDbError this[int index]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal OleDbErrorCollection()
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

	public void CopyTo(Array array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void CopyTo(OleDbError[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public IEnumerator GetEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

using System.Collections;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeNamespaceImportCollection : ICollection, IEnumerable, IList
{
	public int Count
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public CodeNamespaceImport this[int index]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public CodeNamespaceImportCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	int ICollection.get_Count()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
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

	[SpecialName]
	bool IList.get_IsFixedSize()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[SpecialName]
	bool IList.get_IsReadOnly()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[SpecialName]
	object IList.get_Item(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SpecialName]
	void IList.set_Item(int index, object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Add(CodeNamespaceImport value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRange(CodeNamespaceImport[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Clear()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public IEnumerator GetEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	void ICollection.CopyTo(Array array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	IEnumerator IEnumerable.GetEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	int IList.Add(object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	void IList.Clear()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	bool IList.Contains(object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	int IList.IndexOf(object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	void IList.Insert(int index, object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	void IList.Remove(object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	void IList.RemoveAt(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

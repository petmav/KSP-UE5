using System.Collections;
using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeNamespaceCollection : CollectionBase
{
	public CodeNamespace this[int index]
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

	public CodeNamespaceCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeNamespaceCollection(CodeNamespaceCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeNamespaceCollection(CodeNamespace[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int Add(CodeNamespace value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void AddRange(CodeNamespaceCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRange(CodeNamespace[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Contains(CodeNamespace value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void CopyTo(CodeNamespace[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int IndexOf(CodeNamespace value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Insert(int index, CodeNamespace value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Remove(CodeNamespace value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

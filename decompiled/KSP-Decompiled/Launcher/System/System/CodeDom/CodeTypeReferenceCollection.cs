using System.Collections;
using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeTypeReferenceCollection : CollectionBase
{
	public CodeTypeReference this[int index]
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

	public CodeTypeReferenceCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeReferenceCollection(CodeTypeReferenceCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeReferenceCollection(CodeTypeReference[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int Add(CodeTypeReference value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Add(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Add(Type value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRange(CodeTypeReferenceCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRange(CodeTypeReference[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Contains(CodeTypeReference value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void CopyTo(CodeTypeReference[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int IndexOf(CodeTypeReference value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Insert(int index, CodeTypeReference value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Remove(CodeTypeReference value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

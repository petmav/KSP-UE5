using System.Collections;
using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeTypeParameterCollection : CollectionBase
{
	public CodeTypeParameter this[int index]
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

	public CodeTypeParameterCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeParameterCollection(CodeTypeParameterCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeParameterCollection(CodeTypeParameter[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int Add(CodeTypeParameter value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Add(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRange(CodeTypeParameterCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRange(CodeTypeParameter[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Contains(CodeTypeParameter value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void CopyTo(CodeTypeParameter[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int IndexOf(CodeTypeParameter value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Insert(int index, CodeTypeParameter value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Remove(CodeTypeParameter value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using System.Collections;
using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeTypeMemberCollection : CollectionBase
{
	public CodeTypeMember this[int index]
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

	public CodeTypeMemberCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeMemberCollection(CodeTypeMemberCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeMemberCollection(CodeTypeMember[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int Add(CodeTypeMember value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void AddRange(CodeTypeMemberCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRange(CodeTypeMember[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Contains(CodeTypeMember value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void CopyTo(CodeTypeMember[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int IndexOf(CodeTypeMember value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Insert(int index, CodeTypeMember value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Remove(CodeTypeMember value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

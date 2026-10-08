using System.Collections;
using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeAttributeArgumentCollection : CollectionBase
{
	public CodeAttributeArgument this[int index]
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

	public CodeAttributeArgumentCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeAttributeArgumentCollection(CodeAttributeArgumentCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeAttributeArgumentCollection(CodeAttributeArgument[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int Add(CodeAttributeArgument value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void AddRange(CodeAttributeArgumentCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRange(CodeAttributeArgument[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Contains(CodeAttributeArgument value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void CopyTo(CodeAttributeArgument[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int IndexOf(CodeAttributeArgument value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Insert(int index, CodeAttributeArgument value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Remove(CodeAttributeArgument value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using System.Collections;
using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeDirectiveCollection : CollectionBase
{
	public CodeDirective this[int index]
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

	public CodeDirectiveCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeDirectiveCollection(CodeDirectiveCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeDirectiveCollection(CodeDirective[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int Add(CodeDirective value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void AddRange(CodeDirectiveCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRange(CodeDirective[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Contains(CodeDirective value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void CopyTo(CodeDirective[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int IndexOf(CodeDirective value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Insert(int index, CodeDirective value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Remove(CodeDirective value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

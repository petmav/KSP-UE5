using System.Collections;
using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeStatementCollection : CollectionBase
{
	public CodeStatement this[int index]
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

	public CodeStatementCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeStatementCollection(CodeStatementCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeStatementCollection(CodeStatement[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int Add(CodeExpression value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public int Add(CodeStatement value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void AddRange(CodeStatementCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRange(CodeStatement[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Contains(CodeStatement value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void CopyTo(CodeStatement[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int IndexOf(CodeStatement value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Insert(int index, CodeStatement value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Remove(CodeStatement value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

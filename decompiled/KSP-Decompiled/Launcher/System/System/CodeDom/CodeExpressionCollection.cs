using System.Collections;
using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeExpressionCollection : CollectionBase
{
	public CodeExpression this[int index]
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

	public CodeExpressionCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeExpressionCollection(CodeExpressionCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeExpressionCollection(CodeExpression[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int Add(CodeExpression value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void AddRange(CodeExpressionCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRange(CodeExpression[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Contains(CodeExpression value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void CopyTo(CodeExpression[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int IndexOf(CodeExpression value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Insert(int index, CodeExpression value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Remove(CodeExpression value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

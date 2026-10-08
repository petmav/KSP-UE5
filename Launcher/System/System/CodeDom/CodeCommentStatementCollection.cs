using System.Collections;
using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeCommentStatementCollection : CollectionBase
{
	public CodeCommentStatement this[int index]
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

	public CodeCommentStatementCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeCommentStatementCollection(CodeCommentStatementCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeCommentStatementCollection(CodeCommentStatement[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int Add(CodeCommentStatement value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void AddRange(CodeCommentStatementCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRange(CodeCommentStatement[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Contains(CodeCommentStatement value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void CopyTo(CodeCommentStatement[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int IndexOf(CodeCommentStatement value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Insert(int index, CodeCommentStatement value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Remove(CodeCommentStatement value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

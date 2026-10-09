using System.Collections;
using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeCatchClauseCollection : CollectionBase
{
	public CodeCatchClause this[int index]
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

	public CodeCatchClauseCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeCatchClauseCollection(CodeCatchClauseCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeCatchClauseCollection(CodeCatchClause[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int Add(CodeCatchClause value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void AddRange(CodeCatchClauseCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRange(CodeCatchClause[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Contains(CodeCatchClause value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void CopyTo(CodeCatchClause[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int IndexOf(CodeCatchClause value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Insert(int index, CodeCatchClause value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Remove(CodeCatchClause value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using System.Collections;
using System.Security.Permissions;
using Unity;

namespace System.CodeDom.Compiler;

[Serializable]
[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
public class CompilerErrorCollection : CollectionBase
{
	public bool HasErrors
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool HasWarnings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public CompilerError this[int index]
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

	public CompilerErrorCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CompilerErrorCollection(CompilerErrorCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CompilerErrorCollection(CompilerError[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int Add(CompilerError value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void AddRange(CompilerErrorCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRange(CompilerError[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Contains(CompilerError value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void CopyTo(CompilerError[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int IndexOf(CompilerError value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Insert(int index, CompilerError value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Remove(CompilerError value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

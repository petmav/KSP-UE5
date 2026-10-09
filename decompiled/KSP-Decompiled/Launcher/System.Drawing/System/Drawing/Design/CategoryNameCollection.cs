using System.Collections;
using System.Security.Permissions;
using Unity;

namespace System.Drawing.Design;

[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public sealed class CategoryNameCollection : ReadOnlyCollectionBase
{
	public string this[int index]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CategoryNameCollection(CategoryNameCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CategoryNameCollection(string[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Contains(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void CopyTo(string[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int IndexOf(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}
}

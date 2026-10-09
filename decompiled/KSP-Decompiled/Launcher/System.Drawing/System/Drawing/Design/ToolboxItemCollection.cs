using System.Collections;
using System.Security.Permissions;
using Unity;

namespace System.Drawing.Design;

[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public sealed class ToolboxItemCollection : ReadOnlyCollectionBase
{
	public ToolboxItem this[int index]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ToolboxItemCollection(ToolboxItemCollection value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ToolboxItemCollection(ToolboxItem[] value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Contains(ToolboxItem value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void CopyTo(ToolboxItem[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int IndexOf(ToolboxItem value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}
}

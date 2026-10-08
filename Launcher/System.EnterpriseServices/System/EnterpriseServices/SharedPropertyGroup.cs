using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[ComVisible(false)]
public sealed class SharedPropertyGroup
{
	internal SharedPropertyGroup()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SharedProperty CreateProperty(string name, out bool fExists)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public SharedProperty CreatePropertyByPosition(int position, out bool fExists)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public SharedProperty Property(string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public SharedProperty PropertyByPosition(int position)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

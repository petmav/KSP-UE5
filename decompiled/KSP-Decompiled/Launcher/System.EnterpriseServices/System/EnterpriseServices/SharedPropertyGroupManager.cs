using System.Collections;
using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[ComVisible(false)]
public sealed class SharedPropertyGroupManager : IEnumerable
{
	public SharedPropertyGroupManager()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SharedPropertyGroup CreatePropertyGroup(string name, ref PropertyLockMode dwIsoMode, ref PropertyReleaseMode dwRelMode, out bool fExist)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public IEnumerator GetEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public SharedPropertyGroup Group(string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

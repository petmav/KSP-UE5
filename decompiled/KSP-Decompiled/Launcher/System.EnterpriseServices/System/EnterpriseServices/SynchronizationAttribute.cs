using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[AttributeUsage(AttributeTargets.Class, Inherited = true)]
[ComVisible(false)]
public sealed class SynchronizationAttribute : Attribute
{
	public SynchronizationOption Value
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(SynchronizationOption);
		}
	}

	public SynchronizationAttribute()
	{
	}

	public SynchronizationAttribute(SynchronizationOption val)
	{
	}
}

using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[ComVisible(false)]
[AttributeUsage(AttributeTargets.Class, Inherited = true)]
public sealed class ConstructionEnabledAttribute : Attribute
{
	public string Default
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

	public bool Enabled
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public ConstructionEnabledAttribute()
	{
	}

	public ConstructionEnabledAttribute(bool val)
	{
	}
}

using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[ComVisible(false)]
[AttributeUsage(AttributeTargets.Class | AttributeTargets.Interface, Inherited = true, AllowMultiple = true)]
public sealed class InterfaceQueuingAttribute : Attribute
{
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

	public string Interface
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

	public InterfaceQueuingAttribute()
	{
	}

	public InterfaceQueuingAttribute(bool enabled)
	{
	}
}

using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[AttributeUsage(AttributeTargets.Class, Inherited = true)]
[ComVisible(false)]
public sealed class ComponentAccessControlAttribute : Attribute
{
	public bool Value
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public ComponentAccessControlAttribute()
	{
	}

	public ComponentAccessControlAttribute(bool val)
	{
	}
}

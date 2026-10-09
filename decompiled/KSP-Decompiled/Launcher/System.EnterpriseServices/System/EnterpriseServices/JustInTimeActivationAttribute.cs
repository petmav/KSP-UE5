using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[ComVisible(false)]
[AttributeUsage(AttributeTargets.Class, Inherited = true)]
public sealed class JustInTimeActivationAttribute : Attribute
{
	public bool Value
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public JustInTimeActivationAttribute()
	{
	}

	public JustInTimeActivationAttribute(bool val)
	{
	}
}

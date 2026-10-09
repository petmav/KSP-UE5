using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[AttributeUsage(AttributeTargets.Class, Inherited = true)]
[ComVisible(false)]
public sealed class MustRunInClientContextAttribute : Attribute
{
	public bool Value
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public MustRunInClientContextAttribute()
	{
	}

	public MustRunInClientContextAttribute(bool val)
	{
	}
}

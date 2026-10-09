using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[AttributeUsage(AttributeTargets.Method, Inherited = true)]
[ComVisible(false)]
public sealed class AutoCompleteAttribute : Attribute
{
	public bool Value
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public AutoCompleteAttribute()
	{
	}

	public AutoCompleteAttribute(bool val)
	{
	}
}

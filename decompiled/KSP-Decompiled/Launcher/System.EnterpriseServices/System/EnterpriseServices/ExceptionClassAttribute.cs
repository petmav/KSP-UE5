using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[ComVisible(false)]
[AttributeUsage(AttributeTargets.Class, Inherited = true)]
public sealed class ExceptionClassAttribute : Attribute
{
	public string Value
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ExceptionClassAttribute(string name)
	{
	}
}

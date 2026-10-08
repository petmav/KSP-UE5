using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[ComVisible(false)]
[AttributeUsage(AttributeTargets.Assembly, Inherited = true)]
public sealed class ApplicationNameAttribute : Attribute
{
	public string Value
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ApplicationNameAttribute(string name)
	{
	}
}

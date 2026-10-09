using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[AttributeUsage(AttributeTargets.Assembly, Inherited = true)]
[ComVisible(false)]
public sealed class ApplicationIDAttribute : Attribute
{
	public Guid Value
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Guid);
		}
	}

	public ApplicationIDAttribute(string guid)
	{
	}
}

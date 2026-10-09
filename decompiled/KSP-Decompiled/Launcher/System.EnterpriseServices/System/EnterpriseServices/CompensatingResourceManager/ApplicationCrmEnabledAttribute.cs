using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices.CompensatingResourceManager;

[AttributeUsage(AttributeTargets.Assembly, Inherited = true)]
[ComVisible(false)]
[ProgId("System.EnterpriseServices.Crm.ApplicationCrmEnabledAttribute")]
public sealed class ApplicationCrmEnabledAttribute : Attribute
{
	public bool Value
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public ApplicationCrmEnabledAttribute()
	{
	}

	public ApplicationCrmEnabledAttribute(bool val)
	{
	}
}

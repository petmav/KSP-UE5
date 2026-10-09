using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[AttributeUsage(AttributeTargets.Assembly, Inherited = true)]
[ComVisible(false)]
public sealed class ApplicationActivationAttribute : Attribute
{
	public string SoapMailbox
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

	public string SoapVRoot
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

	public ActivationOption Value
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(ActivationOption);
		}
	}

	public ApplicationActivationAttribute(ActivationOption opt)
	{
	}
}

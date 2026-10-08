using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[AttributeUsage(AttributeTargets.Assembly, Inherited = true)]
[ComVisible(false)]
public sealed class ApplicationAccessControlAttribute : Attribute
{
	public AccessChecksLevelOption AccessChecksLevel
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(AccessChecksLevelOption);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public AuthenticationOption Authentication
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(AuthenticationOption);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public ImpersonationLevelOption ImpersonationLevel
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(ImpersonationLevelOption);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool Value
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

	public ApplicationAccessControlAttribute()
	{
	}

	public ApplicationAccessControlAttribute(bool val)
	{
	}
}

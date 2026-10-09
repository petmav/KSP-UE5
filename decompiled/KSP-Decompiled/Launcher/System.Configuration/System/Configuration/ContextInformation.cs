using Unity;

namespace System.Configuration;

public sealed class ContextInformation
{
	public object HostingContext
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool IsMachineLevel
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	internal ContextInformation()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public object GetSection(string sectionName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

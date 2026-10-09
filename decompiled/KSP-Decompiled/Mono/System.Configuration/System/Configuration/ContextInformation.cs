using Unity;

namespace System.Configuration;

public sealed class ContextInformation
{
	private object ctx;

	private Configuration config;

	public object HostingContext => ctx;

	[System.MonoInternalNote("should this use HostingContext instead?")]
	public bool IsMachineLevel => config.ConfigPath == "machine";

	internal ContextInformation(Configuration config, object ctx)
	{
		this.ctx = ctx;
		this.config = config;
	}

	public object GetSection(string sectionName)
	{
		return config.GetSection(sectionName);
	}

	internal ContextInformation()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

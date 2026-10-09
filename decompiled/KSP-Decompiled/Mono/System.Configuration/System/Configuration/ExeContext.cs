using Unity;

namespace System.Configuration;

public sealed class ExeContext
{
	private string path;

	private ConfigurationUserLevel level;

	public string ExePath => path;

	public ConfigurationUserLevel UserLevel => level;

	internal ExeContext(string path, ConfigurationUserLevel level)
	{
		this.path = path;
		this.level = level;
	}

	internal ExeContext()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using Unity;

namespace System.Configuration;

public sealed class ExeContext
{
	public string ExePath
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ConfigurationUserLevel UserLevel
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(ConfigurationUserLevel);
		}
	}

	internal ExeContext()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using Unity;

namespace System.Configuration;

public class ConfigurationLocation
{
	public string Path
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal ConfigurationLocation()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Configuration OpenConfiguration()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

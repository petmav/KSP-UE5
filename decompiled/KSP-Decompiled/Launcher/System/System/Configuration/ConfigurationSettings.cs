using System.Collections.Specialized;
using Unity;

namespace System.Configuration;

public sealed class ConfigurationSettings
{
	public static NameValueCollection AppSettings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal ConfigurationSettings()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[Obsolete("This method is obsolete, it has been replaced by System.Configuration!System.Configuration.ConfigurationManager.GetSection")]
	public static object GetConfig(string sectionName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

using Unity;

namespace System.Configuration;

public sealed class NameValueConfigurationElement : ConfigurationElement
{
	public string Name
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected internal override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string Value
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

	public NameValueConfigurationElement(string name, string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

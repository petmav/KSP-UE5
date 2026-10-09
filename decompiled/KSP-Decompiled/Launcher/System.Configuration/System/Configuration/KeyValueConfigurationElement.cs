using Unity;

namespace System.Configuration;

public class KeyValueConfigurationElement : ConfigurationElement
{
	public string Key
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

	public KeyValueConfigurationElement(string key, string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal override void Init()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

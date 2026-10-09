using Unity;

namespace System.Configuration;

[ConfigurationCollection(typeof(KeyValueConfigurationElement))]
public class KeyValueConfigurationCollection : ConfigurationElementCollection
{
	public string[] AllKeys
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public new KeyValueConfigurationElement this[string key]
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

	protected override bool ThrowOnDuplicate
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public KeyValueConfigurationCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Add(KeyValueConfigurationElement keyValue)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Add(string key, string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Clear()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override ConfigurationElement CreateNewElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override object GetElementKey(ConfigurationElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Remove(string key)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

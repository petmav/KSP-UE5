using Unity;

namespace System.Configuration;

public sealed class SettingElement : ConfigurationElement
{
	public string Name
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

	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SettingsSerializeAs SerializeAs
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(SettingsSerializeAs);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public SettingValueElement Value
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

	public SettingElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SettingElement(string name, SettingsSerializeAs serializeAs)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using System.Xml;
using Unity;

namespace System.Configuration;

public sealed class AppSettingsSection : ConfigurationSection
{
	public string File
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

	protected internal override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public KeyValueConfigurationCollection Settings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public AppSettingsSection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal override void DeserializeElement(XmlReader reader, bool serializeCollectionKey)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal override object GetRuntimeObject()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected internal override bool IsModified()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected internal override void Reset(ConfigurationElement parentSection)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal override string SerializeSection(ConfigurationElement parentElement, string name, ConfigurationSaveMode saveMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

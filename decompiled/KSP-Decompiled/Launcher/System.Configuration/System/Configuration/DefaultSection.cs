using System.Xml;
using Unity;

namespace System.Configuration;

public sealed class DefaultSection : ConfigurationSection
{
	protected internal override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public DefaultSection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal override void DeserializeSection(XmlReader xmlReader)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
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

	protected internal override void ResetModified()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal override string SerializeSection(ConfigurationElement parentSection, string name, ConfigurationSaveMode saveMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

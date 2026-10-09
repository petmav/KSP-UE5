using System.Xml;

namespace System.Configuration;

public sealed class DefaultSection : ConfigurationSection
{
	private static ConfigurationPropertyCollection properties;

	protected internal override ConfigurationPropertyCollection Properties => properties;

	static DefaultSection()
	{
		properties = new ConfigurationPropertyCollection();
	}

	protected internal override void DeserializeSection(XmlReader xmlReader)
	{
		if (base.RawXml == null)
		{
			base.RawXml = xmlReader.ReadOuterXml();
		}
		else
		{
			xmlReader.Skip();
		}
	}

	[System.MonoTODO]
	protected internal override bool IsModified()
	{
		return base.IsModified();
	}

	[System.MonoTODO]
	protected internal override void Reset(ConfigurationElement parentSection)
	{
		base.Reset(parentSection);
	}

	[System.MonoTODO]
	protected internal override void ResetModified()
	{
		base.ResetModified();
	}

	[System.MonoTODO]
	protected internal override string SerializeSection(ConfigurationElement parentSection, string name, ConfigurationSaveMode saveMode)
	{
		return base.SerializeSection(parentSection, name, saveMode);
	}
}

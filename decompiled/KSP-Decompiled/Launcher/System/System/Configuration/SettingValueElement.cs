using System.Xml;
using Unity;

namespace System.Configuration;

public sealed class SettingValueElement : ConfigurationElement
{
	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public XmlNode ValueXml
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

	public SettingValueElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void DeserializeElement(XmlReader reader, bool serializeCollectionKey)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override bool IsModified()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected override void Reset(ConfigurationElement parentElement)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void ResetModified()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override bool SerializeToXmlElement(XmlWriter writer, string elementName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected override void Unmerge(ConfigurationElement sourceElement, ConfigurationElement parentElement, ConfigurationSaveMode saveMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

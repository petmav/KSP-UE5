using System.Runtime.Versioning;
using System.Xml;
using Unity;

namespace System.Configuration;

public abstract class ConfigurationSection : ConfigurationElement
{
	public SectionInformation SectionInformation
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected ConfigurationSection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal virtual void DeserializeSection(XmlReader reader)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal virtual object GetRuntimeObject()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected internal override bool IsModified()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected internal override void ResetModified()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal virtual string SerializeSection(ConfigurationElement parentElement, string name, ConfigurationSaveMode saveMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected internal virtual bool ShouldSerializeElementInTargetVersion(ConfigurationElement element, string elementName, FrameworkName targetFramework)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected internal virtual bool ShouldSerializePropertyInTargetVersion(ConfigurationProperty property, string propertyName, FrameworkName targetFramework, ConfigurationElement parentConfigurationElement)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected internal virtual bool ShouldSerializeSectionInTargetVersion(FrameworkName targetFramework)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}

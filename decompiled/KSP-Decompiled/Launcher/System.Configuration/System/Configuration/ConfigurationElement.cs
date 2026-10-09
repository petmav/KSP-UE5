using System.Collections;
using System.Runtime.CompilerServices;
using System.Xml;
using Unity;

namespace System.Configuration;

public abstract class ConfigurationElement
{
	public Configuration CurrentConfiguration
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ElementInformation ElementInformation
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected internal virtual ConfigurationElementProperty ElementProperty
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected ContextInformation EvaluationContext
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected bool HasContext
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	protected internal object this[string propertyName]
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

	public ConfigurationLockCollection LockAllAttributesExcept
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ConfigurationLockCollection LockAllElementsExcept
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ConfigurationLockCollection LockAttributes
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ConfigurationLockCollection LockElements
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool LockItem
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	protected internal virtual ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected ConfigurationElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	protected internal object get_Item(ConfigurationProperty prop)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SpecialName]
	protected internal void set_Item(ConfigurationProperty prop, object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal virtual void DeserializeElement(XmlReader reader, bool serializeCollectionKey)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual string GetTransformedAssemblyString(string assemblyName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected virtual string GetTransformedTypeString(string typeName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected internal virtual void Init()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal virtual void InitializeDefault()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal virtual bool IsModified()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual bool IsReadOnly()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected virtual void ListErrors(IList errorList)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual bool OnDeserializeUnrecognizedAttribute(string name, string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected virtual bool OnDeserializeUnrecognizedElement(string elementName, XmlReader reader)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected virtual object OnRequiredPropertyNotFound(string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected virtual void PostDeserialize()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void PreSerialize(XmlWriter writer)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal virtual void Reset(ConfigurationElement parentElement)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal virtual void ResetModified()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal virtual bool SerializeElement(XmlWriter writer, bool serializeCollectionKey)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected internal virtual bool SerializeToXmlElement(XmlWriter writer, string elementName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected void SetPropertyValue(ConfigurationProperty prop, object value, bool ignoreLocks)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal virtual void SetReadOnly()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal virtual void Unmerge(ConfigurationElement sourceElement, ConfigurationElement parentElement, ConfigurationSaveMode saveMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

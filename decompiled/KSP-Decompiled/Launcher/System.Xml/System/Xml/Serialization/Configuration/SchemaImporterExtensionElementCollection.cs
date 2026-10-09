using System.Configuration;
using System.Runtime.CompilerServices;
using Unity;

namespace System.Xml.Serialization.Configuration;

[ConfigurationCollection(typeof(SchemaImporterExtensionElement))]
public sealed class SchemaImporterExtensionElementCollection : ConfigurationElementCollection
{
	public new SchemaImporterExtensionElement this[string name]
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

	public SchemaImporterExtensionElementCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	public SchemaImporterExtensionElement get_Item(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SpecialName]
	public void set_Item(int index, SchemaImporterExtensionElement value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Add(SchemaImporterExtensionElement element)
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

	public int IndexOf(SchemaImporterExtensionElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Remove(string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Remove(SchemaImporterExtensionElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void RemoveAt(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

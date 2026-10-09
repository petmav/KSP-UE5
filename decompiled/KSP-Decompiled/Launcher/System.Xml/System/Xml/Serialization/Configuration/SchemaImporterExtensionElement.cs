using System.Configuration;
using Unity;

namespace System.Xml.Serialization.Configuration;

public sealed class SchemaImporterExtensionElement : ConfigurationElement
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

	public Type Type
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

	public SchemaImporterExtensionElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SchemaImporterExtensionElement(string name, string type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SchemaImporterExtensionElement(string name, Type type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

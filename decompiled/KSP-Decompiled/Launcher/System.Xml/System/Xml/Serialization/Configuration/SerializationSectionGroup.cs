using System.Configuration;
using Unity;

namespace System.Xml.Serialization.Configuration;

public sealed class SerializationSectionGroup : ConfigurationSectionGroup
{
	public DateTimeSerializationSection DateTimeSerialization
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SchemaImporterExtensionsSection SchemaImporterExtensions
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public XmlSerializerSection XmlSerializer
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SerializationSectionGroup()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

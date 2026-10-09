using System.Configuration;

namespace System.Xml.Serialization.Configuration;

public sealed class SerializationSectionGroup : ConfigurationSectionGroup
{
	[ConfigurationProperty("schemaImporterExtensions")]
	public SchemaImporterExtensionsSection SchemaImporterExtensions => (SchemaImporterExtensionsSection)base.Sections["schemaImporterExtensions"];

	[ConfigurationProperty("dateTimeSerialization")]
	public DateTimeSerializationSection DateTimeSerialization => (DateTimeSerializationSection)base.Sections["dateTimeSerialization"];

	public XmlSerializerSection XmlSerializer => (XmlSerializerSection)base.Sections["xmlSerializer"];
}

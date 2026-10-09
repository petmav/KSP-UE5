using System.Configuration;
using Unity;

namespace System.Xml.Serialization.Configuration;

public sealed class SchemaImporterExtensionsSection : ConfigurationSection
{
	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SchemaImporterExtensionElementCollection SchemaImporterExtensions
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SchemaImporterExtensionsSection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void InitializeDefault()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

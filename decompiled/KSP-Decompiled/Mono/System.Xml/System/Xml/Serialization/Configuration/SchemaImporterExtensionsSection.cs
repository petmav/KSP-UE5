using System.Configuration;
using System.Xml.Serialization.Advanced;

namespace System.Xml.Serialization.Configuration;

public sealed class SchemaImporterExtensionsSection : ConfigurationSection
{
	private ConfigurationPropertyCollection properties = new ConfigurationPropertyCollection();

	private readonly ConfigurationProperty schemaImporterExtensions = new ConfigurationProperty(null, typeof(SchemaImporterExtensionElementCollection), null, ConfigurationPropertyOptions.IsDefaultCollection);

	protected override ConfigurationPropertyCollection Properties => properties;

	[ConfigurationProperty("", IsDefaultCollection = true)]
	public SchemaImporterExtensionElementCollection SchemaImporterExtensions => (SchemaImporterExtensionElementCollection)base[schemaImporterExtensions];

	internal SchemaImporterExtensionCollection SchemaImporterExtensionsInternal
	{
		get
		{
			SchemaImporterExtensionCollection schemaImporterExtensionCollection = new SchemaImporterExtensionCollection();
			foreach (SchemaImporterExtensionElement schemaImporterExtension in SchemaImporterExtensions)
			{
				schemaImporterExtensionCollection.Add(schemaImporterExtension.Name, schemaImporterExtension.Type);
			}
			return schemaImporterExtensionCollection;
		}
	}

	public SchemaImporterExtensionsSection()
	{
		properties.Add(schemaImporterExtensions);
	}

	private static string GetSqlTypeSchemaImporter(string typeName)
	{
		return "System.Data.SqlTypes." + typeName + ", System.Data, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";
	}

	protected override void InitializeDefault()
	{
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterChar", GetSqlTypeSchemaImporter("TypeCharSchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterNChar", GetSqlTypeSchemaImporter("TypeNCharSchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterVarChar", GetSqlTypeSchemaImporter("TypeVarCharSchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterNVarChar", GetSqlTypeSchemaImporter("TypeNVarCharSchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterText", GetSqlTypeSchemaImporter("TypeTextSchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterNText", GetSqlTypeSchemaImporter("TypeNTextSchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterVarBinary", GetSqlTypeSchemaImporter("TypeVarBinarySchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterBinary", GetSqlTypeSchemaImporter("TypeBinarySchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterImage", GetSqlTypeSchemaImporter("TypeVarImageSchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterDecimal", GetSqlTypeSchemaImporter("TypeDecimalSchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterNumeric", GetSqlTypeSchemaImporter("TypeNumericSchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterBigInt", GetSqlTypeSchemaImporter("TypeBigIntSchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterInt", GetSqlTypeSchemaImporter("TypeIntSchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterSmallInt", GetSqlTypeSchemaImporter("TypeSmallIntSchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterTinyInt", GetSqlTypeSchemaImporter("TypeTinyIntSchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterBit", GetSqlTypeSchemaImporter("TypeBitSchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterFloat", GetSqlTypeSchemaImporter("TypeFloatSchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterReal", GetSqlTypeSchemaImporter("TypeRealSchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterDateTime", GetSqlTypeSchemaImporter("TypeDateTimeSchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterSmallDateTime", GetSqlTypeSchemaImporter("TypeSmallDateTimeSchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterMoney", GetSqlTypeSchemaImporter("TypeMoneySchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterSmallMoney", GetSqlTypeSchemaImporter("TypeSmallMoneySchemaImporterExtension")));
		SchemaImporterExtensions.Add(new SchemaImporterExtensionElement("SqlTypesSchemaImporterUniqueIdentifier", GetSqlTypeSchemaImporter("TypeUniqueIdentifierSchemaImporterExtension")));
	}
}

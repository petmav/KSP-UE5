using System.CodeDom;
using System.CodeDom.Compiler;
using System.Xml.Schema;
using System.Xml.Serialization;
using System.Xml.Serialization.Advanced;
using Unity;

namespace System.Data.SqlTypes;

public class SqlTypesSchemaImporterExtensionHelper : SchemaImporterExtension
{
	protected static readonly string SqlTypesNamespace;

	public SqlTypesSchemaImporterExtensionHelper(string name, string destinationType)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SqlTypesSchemaImporterExtensionHelper(string name, string destinationType, bool direct)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SqlTypesSchemaImporterExtensionHelper(string name, string targetNamespace, string[] references, CodeNamespaceImport[] namespaceImports, string destinationType, bool direct)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override string ImportSchemaType(string name, string xmlNamespace, XmlSchemaObject context, XmlSchemas schemas, XmlSchemaImporter importer, CodeCompileUnit compileUnit, CodeNamespace mainNamespace, CodeGenerationOptions options, CodeDomProvider codeProvider)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override string ImportSchemaType(XmlSchemaType type, XmlSchemaObject context, XmlSchemas schemas, XmlSchemaImporter importer, CodeCompileUnit compileUnit, CodeNamespace mainNamespace, CodeGenerationOptions options, CodeDomProvider codeProvider)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

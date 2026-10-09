using System.CodeDom;
using System.CodeDom.Compiler;
using System.Xml.Schema;
using Unity;

namespace System.Xml.Serialization.Advanced;

public abstract class SchemaImporterExtension
{
	protected SchemaImporterExtension()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual string ImportAnyElement(XmlSchemaAny any, bool mixed, XmlSchemas schemas, XmlSchemaImporter importer, CodeCompileUnit compileUnit, CodeNamespace mainNamespace, CodeGenerationOptions options, CodeDomProvider codeProvider)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual CodeExpression ImportDefaultValue(string value, string type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual string ImportSchemaType(string name, string ns, XmlSchemaObject context, XmlSchemas schemas, XmlSchemaImporter importer, CodeCompileUnit compileUnit, CodeNamespace mainNamespace, CodeGenerationOptions options, CodeDomProvider codeProvider)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual string ImportSchemaType(XmlSchemaType type, XmlSchemaObject context, XmlSchemas schemas, XmlSchemaImporter importer, CodeCompileUnit compileUnit, CodeNamespace mainNamespace, CodeGenerationOptions options, CodeDomProvider codeProvider)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

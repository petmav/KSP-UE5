using System.CodeDom.Compiler;
using Unity;

namespace System.Xml.Serialization;

public class SoapSchemaImporter : SchemaImporter
{
	public SoapSchemaImporter(XmlSchemas schemas)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SoapSchemaImporter(XmlSchemas schemas, CodeGenerationOptions options, CodeDomProvider codeProvider, ImportContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SoapSchemaImporter(XmlSchemas schemas, CodeGenerationOptions options, ImportContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SoapSchemaImporter(XmlSchemas schemas, CodeIdentifiers typeIdentifiers)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SoapSchemaImporter(XmlSchemas schemas, CodeIdentifiers typeIdentifiers, CodeGenerationOptions options)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public XmlTypeMapping ImportDerivedTypeMapping(XmlQualifiedName name, Type baseType, bool baseTypeCanBeIndirect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public XmlMembersMapping ImportMembersMapping(string name, string ns, SoapSchemaMember member)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public XmlMembersMapping ImportMembersMapping(string name, string ns, SoapSchemaMember[] members)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public XmlMembersMapping ImportMembersMapping(string name, string ns, SoapSchemaMember[] members, bool hasWrapperElement)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public XmlMembersMapping ImportMembersMapping(string name, string ns, SoapSchemaMember[] members, bool hasWrapperElement, Type baseType, bool baseTypeCanBeIndirect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

using System.CodeDom;
using System.Collections.Generic;
using System.Xml;
using System.Xml.Schema;
using Unity;

namespace System.Runtime.Serialization;

public class XsdDataContractImporter
{
	public CodeCompileUnit CodeCompileUnit
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ImportOptions Options
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

	public XsdDataContractImporter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public XsdDataContractImporter(CodeCompileUnit codeCompileUnit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool CanImport(XmlSchemaSet schemas)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool CanImport(XmlSchemaSet schemas, ICollection<XmlQualifiedName> typeNames)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool CanImport(XmlSchemaSet schemas, XmlSchemaElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool CanImport(XmlSchemaSet schemas, XmlQualifiedName typeName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public System.CodeDom.CodeTypeReference GetCodeTypeReference(XmlQualifiedName typeName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public System.CodeDom.CodeTypeReference GetCodeTypeReference(XmlQualifiedName typeName, XmlSchemaElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public ICollection<System.CodeDom.CodeTypeReference> GetKnownTypeReferences(XmlQualifiedName typeName)
	{
		//IL_0007: Expected O, but got I4
		Unity.ThrowStub.ThrowNotSupportedException();
		return (ICollection<System.CodeDom.CodeTypeReference>)0;
	}

	public void Import(XmlSchemaSet schemas)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Import(XmlSchemaSet schemas, ICollection<XmlQualifiedName> typeNames)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public XmlQualifiedName Import(XmlSchemaSet schemas, XmlSchemaElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Import(XmlSchemaSet schemas, XmlQualifiedName typeName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

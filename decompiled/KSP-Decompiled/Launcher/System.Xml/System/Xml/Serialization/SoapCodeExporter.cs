using System.CodeDom;
using System.CodeDom.Compiler;
using System.Collections;
using Unity;

namespace System.Xml.Serialization;

public class SoapCodeExporter : CodeExporter
{
	public SoapCodeExporter(CodeNamespace codeNamespace)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SoapCodeExporter(CodeNamespace codeNamespace, CodeCompileUnit codeCompileUnit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SoapCodeExporter(CodeNamespace codeNamespace, CodeCompileUnit codeCompileUnit, CodeDomProvider codeProvider, CodeGenerationOptions options, Hashtable mappings)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SoapCodeExporter(CodeNamespace codeNamespace, CodeCompileUnit codeCompileUnit, CodeGenerationOptions options)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SoapCodeExporter(CodeNamespace codeNamespace, CodeCompileUnit codeCompileUnit, CodeGenerationOptions options, Hashtable mappings)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddMappingMetadata(CodeAttributeDeclarationCollection metadata, XmlMemberMapping member)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddMappingMetadata(CodeAttributeDeclarationCollection metadata, XmlMemberMapping member, bool forceUseMemberName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ExportMembersMapping(XmlMembersMapping xmlMembersMapping)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ExportTypeMapping(XmlTypeMapping xmlTypeMapping)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

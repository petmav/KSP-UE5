using System.CodeDom;
using System.CodeDom.Compiler;
using System.Collections;
using System.ComponentModel;
using System.Diagnostics;
using System.Reflection;
using System.Security.Permissions;

namespace System.Xml.Serialization;

public class SoapCodeExporter : CodeExporter
{
	public SoapCodeExporter(CodeNamespace codeNamespace)
		: base(codeNamespace, null, null, CodeGenerationOptions.GenerateProperties, null)
	{
	}

	public SoapCodeExporter(CodeNamespace codeNamespace, CodeCompileUnit codeCompileUnit)
		: base(codeNamespace, codeCompileUnit, null, CodeGenerationOptions.GenerateProperties, null)
	{
	}

	public SoapCodeExporter(CodeNamespace codeNamespace, CodeCompileUnit codeCompileUnit, CodeGenerationOptions options)
		: base(codeNamespace, codeCompileUnit, null, CodeGenerationOptions.GenerateProperties, null)
	{
	}

	public SoapCodeExporter(CodeNamespace codeNamespace, CodeCompileUnit codeCompileUnit, CodeGenerationOptions options, Hashtable mappings)
		: base(codeNamespace, codeCompileUnit, null, options, mappings)
	{
	}

	public SoapCodeExporter(CodeNamespace codeNamespace, CodeCompileUnit codeCompileUnit, CodeDomProvider codeProvider, CodeGenerationOptions options, Hashtable mappings)
		: base(codeNamespace, codeCompileUnit, codeProvider, options, mappings)
	{
	}

	public void ExportTypeMapping(XmlTypeMapping xmlTypeMapping)
	{
		xmlTypeMapping.CheckShallow();
		CheckScope(xmlTypeMapping.Scope);
		ExportElement(xmlTypeMapping.Accessor);
	}

	public void ExportMembersMapping(XmlMembersMapping xmlMembersMapping)
	{
		xmlMembersMapping.CheckShallow();
		CheckScope(xmlMembersMapping.Scope);
		for (int i = 0; i < xmlMembersMapping.Count; i++)
		{
			ExportElement((ElementAccessor)xmlMembersMapping[i].Accessor);
		}
	}

	private void ExportElement(ElementAccessor element)
	{
		ExportType(element.Mapping);
	}

	private void ExportType(TypeMapping mapping)
	{
		if (mapping.IsReference || base.ExportedMappings[mapping] != null)
		{
			return;
		}
		CodeTypeDeclaration codeTypeDeclaration = null;
		base.ExportedMappings.Add(mapping, mapping);
		if (mapping is EnumMapping)
		{
			codeTypeDeclaration = ExportEnum((EnumMapping)mapping, typeof(SoapEnumAttribute));
		}
		else if (mapping is StructMapping)
		{
			codeTypeDeclaration = ExportStruct((StructMapping)mapping);
		}
		else if (mapping is ArrayMapping)
		{
			EnsureTypesExported(((ArrayMapping)mapping).Elements, null);
		}
		if (codeTypeDeclaration != null)
		{
			codeTypeDeclaration.CustomAttributes.Add(base.GeneratedCodeAttribute);
			codeTypeDeclaration.CustomAttributes.Add(new CodeAttributeDeclaration(typeof(SerializableAttribute).FullName));
			if (!codeTypeDeclaration.IsEnum)
			{
				codeTypeDeclaration.CustomAttributes.Add(new CodeAttributeDeclaration(typeof(DebuggerStepThroughAttribute).FullName));
				codeTypeDeclaration.CustomAttributes.Add(new CodeAttributeDeclaration(typeof(DesignerCategoryAttribute).FullName, new CodeAttributeArgument(new CodePrimitiveExpression("code"))));
			}
			AddTypeMetadata(codeTypeDeclaration.CustomAttributes, typeof(SoapTypeAttribute), mapping.TypeDesc.Name, Accessor.UnescapeName(mapping.TypeName), mapping.Namespace, mapping.IncludeInSchema);
			base.ExportedClasses.Add(mapping, codeTypeDeclaration);
		}
	}

	private CodeTypeDeclaration ExportStruct(StructMapping mapping)
	{
		if (mapping.TypeDesc.IsRoot)
		{
			ExportRoot(mapping, typeof(SoapIncludeAttribute));
			return null;
		}
		if (!mapping.IncludeInSchema)
		{
			return null;
		}
		string name = mapping.TypeDesc.Name;
		string text = ((mapping.TypeDesc.BaseTypeDesc == null) ? string.Empty : mapping.TypeDesc.BaseTypeDesc.Name);
		CodeTypeDeclaration codeTypeDeclaration = new CodeTypeDeclaration(name);
		codeTypeDeclaration.IsPartial = base.CodeProvider.Supports(GeneratorSupport.PartialTypes);
		codeTypeDeclaration.Comments.Add(new CodeCommentStatement(Res.GetString("<remarks/>"), docComment: true));
		base.CodeNamespace.Types.Add(codeTypeDeclaration);
		if (text != null && text.Length > 0)
		{
			codeTypeDeclaration.BaseTypes.Add(text);
		}
		else
		{
			AddPropertyChangedNotifier(codeTypeDeclaration);
		}
		codeTypeDeclaration.TypeAttributes |= TypeAttributes.Public;
		if (mapping.TypeDesc.IsAbstract)
		{
			codeTypeDeclaration.TypeAttributes |= TypeAttributes.Abstract;
		}
		CodeExporter.AddIncludeMetadata(codeTypeDeclaration.CustomAttributes, mapping, typeof(SoapIncludeAttribute));
		if (base.GenerateProperties)
		{
			for (int i = 0; i < mapping.Members.Length; i++)
			{
				ExportProperty(codeTypeDeclaration, mapping.Members[i], mapping.Scope);
			}
		}
		else
		{
			for (int j = 0; j < mapping.Members.Length; j++)
			{
				ExportMember(codeTypeDeclaration, mapping.Members[j]);
			}
		}
		for (int k = 0; k < mapping.Members.Length; k++)
		{
			EnsureTypesExported(mapping.Members[k].Elements, null);
		}
		if (mapping.BaseMapping != null)
		{
			ExportType(mapping.BaseMapping);
		}
		ExportDerivedStructs(mapping);
		CodeGenerator.ValidateIdentifiers(codeTypeDeclaration);
		return codeTypeDeclaration;
	}

	[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
	internal override void ExportDerivedStructs(StructMapping mapping)
	{
		for (StructMapping structMapping = mapping.DerivedMappings; structMapping != null; structMapping = structMapping.NextDerivedMapping)
		{
			ExportType(structMapping);
		}
	}

	public void AddMappingMetadata(CodeAttributeDeclarationCollection metadata, XmlMemberMapping member, bool forceUseMemberName)
	{
		AddMemberMetadata(metadata, member.Mapping, forceUseMemberName);
	}

	public void AddMappingMetadata(CodeAttributeDeclarationCollection metadata, XmlMemberMapping member)
	{
		AddMemberMetadata(metadata, member.Mapping, forceUseMemberName: false);
	}

	private void AddElementMetadata(CodeAttributeDeclarationCollection metadata, string elementName, TypeDesc typeDesc, bool isNullable)
	{
		CodeAttributeDeclaration codeAttributeDeclaration = new CodeAttributeDeclaration(typeof(SoapElementAttribute).FullName);
		if (elementName != null)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument(new CodePrimitiveExpression(elementName)));
		}
		if (typeDesc != null && typeDesc.IsAmbiguousDataType)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument("DataType", new CodePrimitiveExpression(typeDesc.DataType.Name)));
		}
		if (isNullable)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument("IsNullable", new CodePrimitiveExpression(true)));
		}
		metadata.Add(codeAttributeDeclaration);
	}

	private void AddMemberMetadata(CodeAttributeDeclarationCollection metadata, MemberMapping member, bool forceUseMemberName)
	{
		if (member.Elements.Length != 0)
		{
			ElementAccessor elementAccessor = member.Elements[0];
			TypeMapping mapping = elementAccessor.Mapping;
			string text = Accessor.UnescapeName(elementAccessor.Name);
			bool flag = text == member.Name && !forceUseMemberName;
			if (!flag || mapping.TypeDesc.IsAmbiguousDataType || elementAccessor.IsNullable)
			{
				AddElementMetadata(metadata, flag ? null : text, mapping.TypeDesc.IsAmbiguousDataType ? mapping.TypeDesc : null, elementAccessor.IsNullable);
			}
		}
	}

	private void ExportMember(CodeTypeDeclaration codeClass, MemberMapping member)
	{
		CodeMemberField codeMemberField = new CodeMemberField(member.GetTypeName(base.CodeProvider), member.Name);
		codeMemberField.Attributes = (codeMemberField.Attributes & (MemberAttributes)(-61441)) | MemberAttributes.Public;
		codeMemberField.Comments.Add(new CodeCommentStatement(Res.GetString("<remarks/>"), docComment: true));
		codeClass.Members.Add(codeMemberField);
		AddMemberMetadata(codeMemberField.CustomAttributes, member, forceUseMemberName: false);
		if (member.CheckSpecified != 0)
		{
			codeMemberField = new CodeMemberField(typeof(bool).FullName, member.Name + "Specified");
			codeMemberField.Attributes = (codeMemberField.Attributes & (MemberAttributes)(-61441)) | MemberAttributes.Public;
			codeMemberField.Comments.Add(new CodeCommentStatement(Res.GetString("<remarks/>"), docComment: true));
			CodeAttributeDeclaration value = new CodeAttributeDeclaration(typeof(SoapIgnoreAttribute).FullName);
			codeMemberField.CustomAttributes.Add(value);
			codeClass.Members.Add(codeMemberField);
		}
	}

	private void ExportProperty(CodeTypeDeclaration codeClass, MemberMapping member, CodeIdentifiers memberScope)
	{
		string text = memberScope.AddUnique(CodeExporter.MakeFieldName(member.Name), member);
		string typeName = member.GetTypeName(base.CodeProvider);
		CodeMemberField codeMemberField = new CodeMemberField(typeName, text);
		codeMemberField.Attributes = MemberAttributes.Private;
		codeClass.Members.Add(codeMemberField);
		CodeMemberProperty codeMemberProperty = CreatePropertyDeclaration(codeMemberField, member.Name, typeName);
		codeMemberProperty.Comments.Add(new CodeCommentStatement(Res.GetString("<remarks/>"), docComment: true));
		AddMemberMetadata(codeMemberProperty.CustomAttributes, member, forceUseMemberName: false);
		codeClass.Members.Add(codeMemberProperty);
		if (member.CheckSpecified != 0)
		{
			codeMemberField = new CodeMemberField(typeof(bool).FullName, text + "Specified");
			codeMemberField.Attributes = MemberAttributes.Private;
			codeClass.Members.Add(codeMemberField);
			codeMemberProperty = CreatePropertyDeclaration(codeMemberField, member.Name + "Specified", typeof(bool).FullName);
			codeMemberProperty.Comments.Add(new CodeCommentStatement(Res.GetString("<remarks/>"), docComment: true));
			CodeAttributeDeclaration value = new CodeAttributeDeclaration(typeof(SoapIgnoreAttribute).FullName);
			codeMemberProperty.CustomAttributes.Add(value);
			codeClass.Members.Add(codeMemberProperty);
		}
	}

	internal override void EnsureTypesExported(Accessor[] accessors, string ns)
	{
		if (accessors != null)
		{
			for (int i = 0; i < accessors.Length; i++)
			{
				ExportType(accessors[i].Mapping);
			}
		}
	}
}

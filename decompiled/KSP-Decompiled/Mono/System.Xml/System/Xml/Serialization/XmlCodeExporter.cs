using System.CodeDom;
using System.CodeDom.Compiler;
using System.Collections;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.Reflection;
using System.Security.Permissions;
using System.Xml.Schema;
using System.Xml.Serialization.Advanced;

namespace System.Xml.Serialization;

public class XmlCodeExporter : CodeExporter
{
	public XmlCodeExporter(CodeNamespace codeNamespace)
		: base(codeNamespace, null, null, CodeGenerationOptions.GenerateProperties, null)
	{
	}

	public XmlCodeExporter(CodeNamespace codeNamespace, CodeCompileUnit codeCompileUnit)
		: base(codeNamespace, codeCompileUnit, null, CodeGenerationOptions.GenerateProperties, null)
	{
	}

	public XmlCodeExporter(CodeNamespace codeNamespace, CodeCompileUnit codeCompileUnit, CodeGenerationOptions options)
		: base(codeNamespace, codeCompileUnit, null, options, null)
	{
	}

	public XmlCodeExporter(CodeNamespace codeNamespace, CodeCompileUnit codeCompileUnit, CodeGenerationOptions options, Hashtable mappings)
		: base(codeNamespace, codeCompileUnit, null, options, mappings)
	{
	}

	public XmlCodeExporter(CodeNamespace codeNamespace, CodeCompileUnit codeCompileUnit, CodeDomProvider codeProvider, CodeGenerationOptions options, Hashtable mappings)
		: base(codeNamespace, codeCompileUnit, codeProvider, options, mappings)
	{
	}

	public void ExportTypeMapping(XmlTypeMapping xmlTypeMapping)
	{
		xmlTypeMapping.CheckShallow();
		CheckScope(xmlTypeMapping.Scope);
		if (xmlTypeMapping.Accessor.Any)
		{
			throw new InvalidOperationException(Res.GetString("Cannot use wildcards at the top level of a schema."));
		}
		ExportElement(xmlTypeMapping.Accessor);
	}

	public void ExportMembersMapping(XmlMembersMapping xmlMembersMapping)
	{
		xmlMembersMapping.CheckShallow();
		CheckScope(xmlMembersMapping.Scope);
		for (int i = 0; i < xmlMembersMapping.Count; i++)
		{
			AccessorMapping mapping = xmlMembersMapping[i].Mapping;
			if (mapping.Xmlns != null)
			{
				continue;
			}
			if (mapping.Attribute != null)
			{
				ExportType(mapping.Attribute.Mapping, Accessor.UnescapeName(mapping.Attribute.Name), mapping.Attribute.Namespace, null, checkReference: false);
			}
			if (mapping.Elements != null)
			{
				for (int j = 0; j < mapping.Elements.Length; j++)
				{
					ElementAccessor elementAccessor = mapping.Elements[j];
					ExportType(elementAccessor.Mapping, Accessor.UnescapeName(elementAccessor.Name), elementAccessor.Namespace, null, checkReference: false);
				}
			}
			if (mapping.Text != null)
			{
				ExportType(mapping.Text.Mapping, Accessor.UnescapeName(mapping.Text.Name), mapping.Text.Namespace, null, checkReference: false);
			}
		}
	}

	private void ExportElement(ElementAccessor element)
	{
		ExportType(element.Mapping, Accessor.UnescapeName(element.Name), element.Namespace, element, checkReference: true);
	}

	private void ExportType(TypeMapping mapping, string ns)
	{
		ExportType(mapping, null, ns, null, checkReference: true);
	}

	private void ExportType(TypeMapping mapping, string name, string ns, ElementAccessor rootElement, bool checkReference)
	{
		if ((mapping.IsReference && mapping.Namespace != "http://schemas.xmlsoap.org/soap/encoding/") || (mapping is StructMapping && checkReference && ((StructMapping)mapping).ReferencedByTopLevelElement && rootElement == null))
		{
			return;
		}
		if (mapping is ArrayMapping && rootElement != null && rootElement.IsTopLevelInSchema && ((ArrayMapping)mapping).TopLevelMapping != null)
		{
			mapping = ((ArrayMapping)mapping).TopLevelMapping;
		}
		CodeTypeDeclaration codeTypeDeclaration = null;
		if (base.ExportedMappings[mapping] == null)
		{
			base.ExportedMappings.Add(mapping, mapping);
			if (mapping.TypeDesc.IsMappedType)
			{
				codeTypeDeclaration = mapping.TypeDesc.ExtendedType.ExportTypeDefinition(base.CodeNamespace, base.CodeCompileUnit);
			}
			else if (mapping is EnumMapping)
			{
				codeTypeDeclaration = ExportEnum((EnumMapping)mapping, typeof(XmlEnumAttribute));
			}
			else if (mapping is StructMapping)
			{
				codeTypeDeclaration = ExportStruct((StructMapping)mapping);
			}
			else if (mapping is ArrayMapping)
			{
				EnsureTypesExported(((ArrayMapping)mapping).Elements, ns);
			}
			if (codeTypeDeclaration != null)
			{
				if (!mapping.TypeDesc.IsMappedType)
				{
					codeTypeDeclaration.CustomAttributes.Add(base.GeneratedCodeAttribute);
					codeTypeDeclaration.CustomAttributes.Add(new CodeAttributeDeclaration(typeof(SerializableAttribute).FullName));
					if (!codeTypeDeclaration.IsEnum)
					{
						codeTypeDeclaration.CustomAttributes.Add(new CodeAttributeDeclaration(typeof(DebuggerStepThroughAttribute).FullName));
						codeTypeDeclaration.CustomAttributes.Add(new CodeAttributeDeclaration(typeof(DesignerCategoryAttribute).FullName, new CodeAttributeArgument(new CodePrimitiveExpression("code"))));
					}
					AddTypeMetadata(codeTypeDeclaration.CustomAttributes, typeof(XmlTypeAttribute), mapping.TypeDesc.Name, Accessor.UnescapeName(mapping.TypeName), mapping.Namespace, mapping.IncludeInSchema);
				}
				else if (CodeExporter.FindAttributeDeclaration(typeof(GeneratedCodeAttribute), codeTypeDeclaration.CustomAttributes) == null)
				{
					codeTypeDeclaration.CustomAttributes.Add(base.GeneratedCodeAttribute);
				}
				base.ExportedClasses.Add(mapping, codeTypeDeclaration);
			}
		}
		else
		{
			codeTypeDeclaration = (CodeTypeDeclaration)base.ExportedClasses[mapping];
		}
		if (codeTypeDeclaration != null && rootElement != null)
		{
			AddRootMetadata(codeTypeDeclaration.CustomAttributes, mapping, name, ns, rootElement);
		}
	}

	private void AddRootMetadata(CodeAttributeDeclarationCollection metadata, TypeMapping typeMapping, string name, string ns, ElementAccessor rootElement)
	{
		string fullName = typeof(XmlRootAttribute).FullName;
		foreach (CodeAttributeDeclaration metadatum in metadata)
		{
			if (metadatum.Name == fullName)
			{
				return;
			}
		}
		CodeAttributeDeclaration codeAttributeDeclaration = new CodeAttributeDeclaration(fullName);
		if (typeMapping.TypeDesc.Name != name)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument(new CodePrimitiveExpression(name)));
		}
		if (ns != null)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument("Namespace", new CodePrimitiveExpression(ns)));
		}
		if (typeMapping.TypeDesc != null && typeMapping.TypeDesc.IsAmbiguousDataType)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument("DataType", new CodePrimitiveExpression(typeMapping.TypeDesc.DataType.Name)));
		}
		if ((object)rootElement.IsNullable != null)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument("IsNullable", new CodePrimitiveExpression(rootElement.IsNullable)));
		}
		metadata.Add(codeAttributeDeclaration);
	}

	private CodeAttributeArgument[] GetDefaultValueArguments(PrimitiveMapping mapping, object value, out CodeExpression initExpression)
	{
		initExpression = null;
		if (value == null)
		{
			return null;
		}
		CodeExpression codeExpression = null;
		CodeExpression codeExpression2 = null;
		Type type = value.GetType();
		CodeAttributeArgument[] result = null;
		if (mapping is EnumMapping)
		{
			if (((EnumMapping)mapping).IsFlags)
			{
				string[] array = ((string)value).Split(null);
				for (int i = 0; i < array.Length; i++)
				{
					if (array[i].Length != 0)
					{
						CodeExpression codeExpression3 = new CodeFieldReferenceExpression(new CodeTypeReferenceExpression(mapping.TypeDesc.FullName), array[i]);
						codeExpression = ((codeExpression == null) ? codeExpression3 : new CodeBinaryOperatorExpression(codeExpression, CodeBinaryOperatorType.BitwiseOr, codeExpression3));
					}
				}
			}
			else
			{
				codeExpression = new CodeFieldReferenceExpression(new CodeTypeReferenceExpression(mapping.TypeDesc.FullName), (string)value);
			}
			initExpression = codeExpression;
			result = new CodeAttributeArgument[1]
			{
				new CodeAttributeArgument(codeExpression)
			};
		}
		else if (type == typeof(bool) || type == typeof(int) || type == typeof(string) || type == typeof(double))
		{
			codeExpression = (initExpression = new CodePrimitiveExpression(value));
			result = new CodeAttributeArgument[1]
			{
				new CodeAttributeArgument(codeExpression)
			};
		}
		else if (type == typeof(short) || type == typeof(long) || type == typeof(float) || type == typeof(byte) || type == typeof(decimal))
		{
			codeExpression = new CodePrimitiveExpression(Convert.ToString(value, NumberFormatInfo.InvariantInfo));
			codeExpression2 = new CodeTypeOfExpression(type.FullName);
			result = new CodeAttributeArgument[2]
			{
				new CodeAttributeArgument(codeExpression2),
				new CodeAttributeArgument(codeExpression)
			};
			initExpression = new CodeCastExpression(type.FullName, new CodePrimitiveExpression(value));
		}
		else if (type == typeof(sbyte) || type == typeof(ushort) || type == typeof(uint) || type == typeof(ulong))
		{
			value = CodeExporter.PromoteType(type, value);
			codeExpression = new CodePrimitiveExpression(Convert.ToString(value, NumberFormatInfo.InvariantInfo));
			codeExpression2 = new CodeTypeOfExpression(type.FullName);
			result = new CodeAttributeArgument[2]
			{
				new CodeAttributeArgument(codeExpression2),
				new CodeAttributeArgument(codeExpression)
			};
			initExpression = new CodeCastExpression(type.FullName, new CodePrimitiveExpression(value));
		}
		else if (type == typeof(DateTime))
		{
			DateTime value2 = (DateTime)value;
			string value3;
			long ticks;
			if (mapping.TypeDesc.FormatterName == "Date")
			{
				value3 = XmlCustomFormatter.FromDate(value2);
				ticks = new DateTime(value2.Year, value2.Month, value2.Day).Ticks;
			}
			else if (mapping.TypeDesc.FormatterName == "Time")
			{
				value3 = XmlCustomFormatter.FromDateTime(value2);
				ticks = value2.Ticks;
			}
			else
			{
				value3 = XmlCustomFormatter.FromDateTime(value2);
				ticks = value2.Ticks;
			}
			codeExpression = new CodePrimitiveExpression(value3);
			codeExpression2 = new CodeTypeOfExpression(type.FullName);
			result = new CodeAttributeArgument[2]
			{
				new CodeAttributeArgument(codeExpression2),
				new CodeAttributeArgument(codeExpression)
			};
			initExpression = new CodeObjectCreateExpression(new CodeTypeReference(typeof(DateTime)), new CodePrimitiveExpression(ticks));
		}
		else if (type == typeof(Guid))
		{
			codeExpression = new CodePrimitiveExpression(Convert.ToString(value, NumberFormatInfo.InvariantInfo));
			codeExpression2 = new CodeTypeOfExpression(type.FullName);
			result = new CodeAttributeArgument[2]
			{
				new CodeAttributeArgument(codeExpression2),
				new CodeAttributeArgument(codeExpression)
			};
			initExpression = new CodeObjectCreateExpression(new CodeTypeReference(typeof(Guid)), codeExpression);
		}
		if (mapping.TypeDesc.FullName != type.ToString() && !(mapping is EnumMapping))
		{
			initExpression = new CodeCastExpression(mapping.TypeDesc.FullName, initExpression);
		}
		return result;
	}

	private object ImportDefault(TypeMapping mapping, string defaultValue)
	{
		if (defaultValue == null)
		{
			return null;
		}
		if (mapping.IsList)
		{
			string[] array = defaultValue.Trim().Split(null);
			int num = 0;
			for (int i = 0; i < array.Length; i++)
			{
				if (array[i] != null && array[i].Length > 0)
				{
					num++;
				}
			}
			object[] array2 = new object[num];
			num = 0;
			for (int j = 0; j < array.Length; j++)
			{
				if (array[j] != null && array[j].Length > 0)
				{
					array2[num++] = ImportDefaultValue(mapping, array[j]);
				}
			}
			return array2;
		}
		return ImportDefaultValue(mapping, defaultValue);
	}

	private object ImportDefaultValue(TypeMapping mapping, string defaultValue)
	{
		if (defaultValue == null)
		{
			return null;
		}
		if (!(mapping is PrimitiveMapping))
		{
			return DBNull.Value;
		}
		if (mapping is EnumMapping)
		{
			EnumMapping enumMapping = (EnumMapping)mapping;
			ConstantMapping[] constants = enumMapping.Constants;
			if (enumMapping.IsFlags)
			{
				Hashtable hashtable = new Hashtable();
				string[] array = new string[constants.Length];
				long[] array2 = new long[constants.Length];
				for (int i = 0; i < constants.Length; i++)
				{
					array2[i] = (enumMapping.IsFlags ? (1L << i) : i);
					array[i] = constants[i].Name;
					hashtable.Add(constants[i].Name, array2[i]);
				}
				return XmlCustomFormatter.FromEnum(XmlCustomFormatter.ToEnum(defaultValue, hashtable, enumMapping.TypeName, validate: true), array, array2, enumMapping.TypeDesc.FullName);
			}
			for (int j = 0; j < constants.Length; j++)
			{
				if (constants[j].XmlName == defaultValue)
				{
					return constants[j].Name;
				}
			}
			throw new InvalidOperationException(Res.GetString("Value '{0}' cannot be converted to {1}.", defaultValue, enumMapping.TypeDesc.FullName));
		}
		PrimitiveMapping primitiveMapping = (PrimitiveMapping)mapping;
		if (!primitiveMapping.TypeDesc.HasCustomFormatter)
		{
			if (primitiveMapping.TypeDesc.FormatterName == "String")
			{
				return defaultValue;
			}
			if (primitiveMapping.TypeDesc.FormatterName == "DateTime")
			{
				return XmlCustomFormatter.ToDateTime(defaultValue);
			}
			Type typeFromHandle = typeof(XmlConvert);
			MethodInfo method = typeFromHandle.GetMethod("To" + primitiveMapping.TypeDesc.FormatterName, new Type[1] { typeof(string) });
			if (method != null)
			{
				return method.Invoke(typeFromHandle, new object[1] { defaultValue });
			}
		}
		else if (primitiveMapping.TypeDesc.HasDefaultSupport)
		{
			return XmlCustomFormatter.ToDefaultValue(defaultValue, primitiveMapping.TypeDesc.FormatterName);
		}
		return DBNull.Value;
	}

	private void AddDefaultValueAttribute(CodeMemberField field, CodeAttributeDeclarationCollection metadata, object defaultValue, TypeMapping mapping, CodeCommentStatementCollection comments, TypeDesc memberTypeDesc, Accessor accessor, CodeConstructor ctor)
	{
		string text = (accessor.IsFixed ? "fixed" : "default");
		if (!memberTypeDesc.HasDefaultSupport)
		{
			if (comments != null && defaultValue is string)
			{
				DropDefaultAttribute(accessor, comments, memberTypeDesc.FullName);
				CodeExporter.AddWarningComment(comments, Res.GetString("'{0}' attribute on items of type '{1}' is not supported in this version of the .Net Framework.  Ignoring {0}='{2}' attribute.", text, mapping.TypeName, defaultValue.ToString()));
			}
			return;
		}
		if (memberTypeDesc.IsArrayLike && accessor is ElementAccessor)
		{
			if (comments != null && defaultValue is string)
			{
				DropDefaultAttribute(accessor, comments, memberTypeDesc.FullName);
				CodeExporter.AddWarningComment(comments, Res.GetString("'{0}' attribute on array-like elements is not supported in this version of the .Net Framework.  Ignoring {0}='{1}' attribute on element name='{2}'.", text, defaultValue.ToString(), ((ElementAccessor)accessor).Name));
			}
			return;
		}
		if (mapping.TypeDesc.IsMappedType && field != null && defaultValue is string)
		{
			SchemaImporterExtension extension = mapping.TypeDesc.ExtendedType.Extension;
			CodeExpression codeExpression = extension.ImportDefaultValue((string)defaultValue, mapping.TypeDesc.FullName);
			if (codeExpression != null)
			{
				if (ctor != null)
				{
					AddInitializationStatement(ctor, field, codeExpression);
				}
				else
				{
					field.InitExpression = extension.ImportDefaultValue((string)defaultValue, mapping.TypeDesc.FullName);
				}
			}
			if (comments != null)
			{
				DropDefaultAttribute(accessor, comments, mapping.TypeDesc.FullName);
				if (codeExpression == null)
				{
					CodeExporter.AddWarningComment(comments, Res.GetString("Schema importer extension {0} failed to parse '{1}'='{2}' attribute of type {3} from namespace='{4}'.", extension.GetType().FullName, text, (string)defaultValue, mapping.TypeName, mapping.Namespace));
				}
			}
			return;
		}
		object obj = null;
		if (defaultValue is string || defaultValue == null)
		{
			obj = ImportDefault(mapping, (string)defaultValue);
		}
		if (obj == null)
		{
			return;
		}
		if (!(mapping is PrimitiveMapping))
		{
			if (comments != null)
			{
				DropDefaultAttribute(accessor, comments, memberTypeDesc.FullName);
				CodeExporter.AddWarningComment(comments, Res.GetString("'{0}' attribute supported only for primitive types.  Ignoring {0}='{1}' attribute.", text, defaultValue.ToString()));
			}
			return;
		}
		PrimitiveMapping primitiveMapping = (PrimitiveMapping)mapping;
		if (comments != null && !primitiveMapping.TypeDesc.HasDefaultSupport && primitiveMapping.TypeDesc.IsMappedType)
		{
			DropDefaultAttribute(accessor, comments, primitiveMapping.TypeDesc.FullName);
			return;
		}
		if (obj == DBNull.Value)
		{
			if (comments != null)
			{
				CodeExporter.AddWarningComment(comments, Res.GetString("'{0}' attribute on items of type '{1}' is not supported in this version of the .Net Framework.  Ignoring {0}='{2}' attribute.", text, primitiveMapping.TypeName, defaultValue.ToString()));
			}
			return;
		}
		CodeAttributeArgument[] array = null;
		CodeExpression initExpression = null;
		if (primitiveMapping.IsList)
		{
			object[] array2 = (object[])obj;
			CodeExpression[] array3 = new CodeExpression[array2.Length];
			for (int i = 0; i < array2.Length; i++)
			{
				GetDefaultValueArguments(primitiveMapping, array2[i], out array3[i]);
			}
			initExpression = new CodeArrayCreateExpression(field.Type, array3);
		}
		else
		{
			array = GetDefaultValueArguments(primitiveMapping, obj, out initExpression);
		}
		if (field != null)
		{
			if (ctor != null)
			{
				AddInitializationStatement(ctor, field, initExpression);
			}
			else
			{
				field.InitExpression = initExpression;
			}
		}
		if (array != null && primitiveMapping.TypeDesc.HasDefaultSupport && accessor.IsOptional && !accessor.IsFixed)
		{
			CodeAttributeDeclaration value = new CodeAttributeDeclaration(typeof(DefaultValueAttribute).FullName, array);
			metadata.Add(value);
		}
		else if (comments != null)
		{
			DropDefaultAttribute(accessor, comments, memberTypeDesc.FullName);
		}
	}

	private static void AddInitializationStatement(CodeConstructor ctor, CodeMemberField field, CodeExpression init)
	{
		CodeAssignStatement codeAssignStatement = new CodeAssignStatement();
		codeAssignStatement.Left = new CodeFieldReferenceExpression(new CodeThisReferenceExpression(), field.Name);
		codeAssignStatement.Right = init;
		ctor.Statements.Add(codeAssignStatement);
	}

	private static void DropDefaultAttribute(Accessor accessor, CodeCommentStatementCollection comments, string type)
	{
		if (!accessor.IsFixed && accessor.IsOptional)
		{
			CodeExporter.AddWarningComment(comments, Res.GetString("DefaultValue attribute on members of type {0} is not supported in this version of the .Net Framework.", type));
		}
	}

	private CodeTypeDeclaration ExportStruct(StructMapping mapping)
	{
		if (mapping.TypeDesc.IsRoot)
		{
			ExportRoot(mapping, typeof(XmlIncludeAttribute));
			return null;
		}
		string name = mapping.TypeDesc.Name;
		string text = ((mapping.TypeDesc.BaseTypeDesc == null || mapping.TypeDesc.BaseTypeDesc.IsRoot) ? string.Empty : mapping.TypeDesc.BaseTypeDesc.FullName);
		CodeTypeDeclaration codeTypeDeclaration = new CodeTypeDeclaration(name);
		codeTypeDeclaration.IsPartial = base.CodeProvider.Supports(GeneratorSupport.PartialTypes);
		codeTypeDeclaration.Comments.Add(new CodeCommentStatement(Res.GetString("<remarks/>"), docComment: true));
		base.CodeNamespace.Types.Add(codeTypeDeclaration);
		CodeConstructor codeConstructor = new CodeConstructor();
		codeConstructor.Attributes = (codeConstructor.Attributes & (MemberAttributes)(-61441)) | MemberAttributes.Public;
		codeTypeDeclaration.Members.Add(codeConstructor);
		if (mapping.TypeDesc.IsAbstract)
		{
			codeConstructor.Attributes |= MemberAttributes.Abstract;
		}
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
		CodeExporter.AddIncludeMetadata(codeTypeDeclaration.CustomAttributes, mapping, typeof(XmlIncludeAttribute));
		if (mapping.IsSequence)
		{
			int num = 0;
			for (int i = 0; i < mapping.Members.Length; i++)
			{
				MemberMapping memberMapping = mapping.Members[i];
				if (memberMapping.IsParticle && memberMapping.SequenceId < 0)
				{
					memberMapping.SequenceId = num++;
				}
			}
		}
		if (base.GenerateProperties)
		{
			for (int j = 0; j < mapping.Members.Length; j++)
			{
				ExportProperty(codeTypeDeclaration, mapping.Members[j], mapping.Namespace, mapping.Scope, codeConstructor);
			}
		}
		else
		{
			for (int k = 0; k < mapping.Members.Length; k++)
			{
				ExportMember(codeTypeDeclaration, mapping.Members[k], mapping.Namespace, codeConstructor);
			}
		}
		for (int l = 0; l < mapping.Members.Length; l++)
		{
			if (mapping.Members[l].Xmlns == null)
			{
				EnsureTypesExported(mapping.Members[l].Elements, mapping.Namespace);
				EnsureTypesExported(mapping.Members[l].Attribute, mapping.Namespace);
				EnsureTypesExported(mapping.Members[l].Text, mapping.Namespace);
			}
		}
		if (mapping.BaseMapping != null)
		{
			ExportType(mapping.BaseMapping, null, mapping.Namespace, null, checkReference: false);
		}
		ExportDerivedStructs(mapping);
		CodeGenerator.ValidateIdentifiers(codeTypeDeclaration);
		if (codeConstructor.Statements.Count == 0)
		{
			codeTypeDeclaration.Members.Remove(codeConstructor);
		}
		return codeTypeDeclaration;
	}

	[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
	internal override void ExportDerivedStructs(StructMapping mapping)
	{
		for (StructMapping structMapping = mapping.DerivedMappings; structMapping != null; structMapping = structMapping.NextDerivedMapping)
		{
			ExportType(structMapping, mapping.Namespace);
		}
	}

	public void AddMappingMetadata(CodeAttributeDeclarationCollection metadata, XmlTypeMapping mapping, string ns)
	{
		mapping.CheckShallow();
		CheckScope(mapping.Scope);
		if (!(mapping.Mapping is StructMapping) && !(mapping.Mapping is EnumMapping))
		{
			AddRootMetadata(metadata, mapping.Mapping, Accessor.UnescapeName(mapping.Accessor.Name), mapping.Accessor.Namespace, mapping.Accessor);
		}
	}

	public void AddMappingMetadata(CodeAttributeDeclarationCollection metadata, XmlMemberMapping member, string ns, bool forceUseMemberName)
	{
		AddMemberMetadata(null, metadata, member.Mapping, ns, forceUseMemberName, null, null);
	}

	public void AddMappingMetadata(CodeAttributeDeclarationCollection metadata, XmlMemberMapping member, string ns)
	{
		AddMemberMetadata(null, metadata, member.Mapping, ns, forceUseMemberName: false, null, null);
	}

	private void ExportArrayElements(CodeAttributeDeclarationCollection metadata, ArrayMapping array, string ns, TypeDesc elementTypeDesc, int nestingLevel)
	{
		for (int i = 0; i < array.Elements.Length; i++)
		{
			ElementAccessor elementAccessor = array.Elements[i];
			TypeMapping mapping = elementAccessor.Mapping;
			string text = Accessor.UnescapeName(elementAccessor.Name);
			bool flag = !elementAccessor.Mapping.TypeDesc.IsArray && text == elementAccessor.Mapping.TypeName;
			bool flag2 = mapping.TypeDesc == elementTypeDesc;
			bool flag3 = elementAccessor.Form == XmlSchemaForm.Unqualified || elementAccessor.Namespace == ns;
			bool flag4 = elementAccessor.IsNullable == mapping.TypeDesc.IsNullable;
			bool flag5 = elementAccessor.Form != XmlSchemaForm.Unqualified;
			if (!flag || !flag2 || !flag3 || !flag4 || !flag5 || nestingLevel > 0)
			{
				ExportArrayItem(metadata, flag ? null : text, flag3 ? null : elementAccessor.Namespace, flag2 ? null : mapping.TypeDesc, mapping.TypeDesc, elementAccessor.IsNullable, (!flag5) ? elementAccessor.Form : XmlSchemaForm.None, nestingLevel);
			}
			if (mapping is ArrayMapping)
			{
				ExportArrayElements(metadata, (ArrayMapping)mapping, ns, elementTypeDesc.ArrayElementTypeDesc, nestingLevel + 1);
			}
		}
	}

	private void AddMemberMetadata(CodeMemberField field, CodeAttributeDeclarationCollection metadata, MemberMapping member, string ns, bool forceUseMemberName, CodeCommentStatementCollection comments, CodeConstructor ctor)
	{
		if (member.Xmlns != null)
		{
			CodeAttributeDeclaration value = new CodeAttributeDeclaration(typeof(XmlNamespaceDeclarationsAttribute).FullName);
			metadata.Add(value);
			return;
		}
		if (member.Attribute != null)
		{
			AttributeAccessor attribute = member.Attribute;
			if (attribute.Any)
			{
				ExportAnyAttribute(metadata);
				return;
			}
			TypeMapping mapping = attribute.Mapping;
			string text = Accessor.UnescapeName(attribute.Name);
			bool flag = mapping.TypeDesc == member.TypeDesc || (member.TypeDesc.IsArrayLike && mapping.TypeDesc == member.TypeDesc.ArrayElementTypeDesc);
			bool flag2 = text == member.Name && !forceUseMemberName;
			bool flag3 = attribute.Namespace == ns;
			bool flag4 = attribute.Form != XmlSchemaForm.Qualified;
			ExportAttribute(metadata, flag2 ? null : text, (flag3 || flag4) ? null : attribute.Namespace, flag ? null : mapping.TypeDesc, mapping.TypeDesc, (!flag4) ? attribute.Form : XmlSchemaForm.None);
			AddDefaultValueAttribute(field, metadata, attribute.Default, mapping, comments, member.TypeDesc, attribute, ctor);
			return;
		}
		if (member.Text != null)
		{
			TypeMapping mapping2 = member.Text.Mapping;
			bool flag5 = mapping2.TypeDesc == member.TypeDesc || (member.TypeDesc.IsArrayLike && mapping2.TypeDesc == member.TypeDesc.ArrayElementTypeDesc);
			ExportText(metadata, flag5 ? null : mapping2.TypeDesc, mapping2.TypeDesc.IsAmbiguousDataType ? mapping2.TypeDesc.DataType.Name : null);
		}
		if (member.Elements.Length == 1)
		{
			ElementAccessor elementAccessor = member.Elements[0];
			TypeMapping mapping3 = elementAccessor.Mapping;
			string text2 = Accessor.UnescapeName(elementAccessor.Name);
			bool flag6 = text2 == member.Name && !forceUseMemberName;
			bool flag7 = mapping3 is ArrayMapping;
			bool flag8 = elementAccessor.Namespace == ns;
			bool flag9 = elementAccessor.Form != XmlSchemaForm.Unqualified;
			if (elementAccessor.Any)
			{
				ExportAnyElement(metadata, text2, elementAccessor.Namespace, member.SequenceId);
			}
			else if (flag7)
			{
				_ = mapping3.TypeDesc;
				_ = member.TypeDesc;
				ArrayMapping array = (ArrayMapping)mapping3;
				if (!flag6 || !flag8 || elementAccessor.IsNullable || !flag9 || member.SequenceId != -1)
				{
					ExportArray(metadata, flag6 ? null : text2, flag8 ? null : elementAccessor.Namespace, elementAccessor.IsNullable, (!flag9) ? elementAccessor.Form : XmlSchemaForm.None, member.SequenceId);
				}
				else if (mapping3.TypeDesc.ArrayElementTypeDesc == new TypeScope().GetTypeDesc(typeof(byte)))
				{
					ExportArray(metadata, null, null, isNullable: false, XmlSchemaForm.None, member.SequenceId);
				}
				ExportArrayElements(metadata, array, elementAccessor.Namespace, member.TypeDesc.ArrayElementTypeDesc, 0);
			}
			else
			{
				bool flag10 = mapping3.TypeDesc == member.TypeDesc || (member.TypeDesc.IsArrayLike && mapping3.TypeDesc == member.TypeDesc.ArrayElementTypeDesc);
				if (member.TypeDesc.IsArrayLike)
				{
					flag6 = false;
				}
				ExportElement(metadata, flag6 ? null : text2, flag8 ? null : elementAccessor.Namespace, flag10 ? null : mapping3.TypeDesc, mapping3.TypeDesc, elementAccessor.IsNullable, (!flag9) ? elementAccessor.Form : XmlSchemaForm.None, member.SequenceId);
			}
			AddDefaultValueAttribute(field, metadata, elementAccessor.Default, mapping3, comments, member.TypeDesc, elementAccessor, ctor);
		}
		else
		{
			for (int i = 0; i < member.Elements.Length; i++)
			{
				ElementAccessor elementAccessor2 = member.Elements[i];
				string name = Accessor.UnescapeName(elementAccessor2.Name);
				bool flag11 = elementAccessor2.Namespace == ns;
				if (elementAccessor2.Any)
				{
					ExportAnyElement(metadata, name, elementAccessor2.Namespace, member.SequenceId);
					continue;
				}
				bool flag12 = elementAccessor2.Form != XmlSchemaForm.Unqualified;
				ExportElement(metadata, name, flag11 ? null : elementAccessor2.Namespace, elementAccessor2.Mapping.TypeDesc, elementAccessor2.Mapping.TypeDesc, elementAccessor2.IsNullable, (!flag12) ? elementAccessor2.Form : XmlSchemaForm.None, member.SequenceId);
			}
		}
		if (member.ChoiceIdentifier != null)
		{
			CodeAttributeDeclaration codeAttributeDeclaration = new CodeAttributeDeclaration(typeof(XmlChoiceIdentifierAttribute).FullName);
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument(new CodePrimitiveExpression(member.ChoiceIdentifier.MemberName)));
			metadata.Add(codeAttributeDeclaration);
		}
		if (member.Ignore)
		{
			CodeAttributeDeclaration value2 = new CodeAttributeDeclaration(typeof(XmlIgnoreAttribute).FullName);
			metadata.Add(value2);
		}
	}

	private void ExportMember(CodeTypeDeclaration codeClass, MemberMapping member, string ns, CodeConstructor ctor)
	{
		CodeMemberField codeMemberField = new CodeMemberField(member.GetTypeName(base.CodeProvider), member.Name);
		codeMemberField.Attributes = (codeMemberField.Attributes & (MemberAttributes)(-61441)) | MemberAttributes.Public;
		codeMemberField.Comments.Add(new CodeCommentStatement(Res.GetString("<remarks/>"), docComment: true));
		codeClass.Members.Add(codeMemberField);
		AddMemberMetadata(codeMemberField, codeMemberField.CustomAttributes, member, ns, forceUseMemberName: false, codeMemberField.Comments, ctor);
		if (member.CheckSpecified != 0)
		{
			codeMemberField = new CodeMemberField(typeof(bool).FullName, member.Name + "Specified");
			codeMemberField.Attributes = (codeMemberField.Attributes & (MemberAttributes)(-61441)) | MemberAttributes.Public;
			codeMemberField.Comments.Add(new CodeCommentStatement(Res.GetString("<remarks/>"), docComment: true));
			CodeAttributeDeclaration value = new CodeAttributeDeclaration(typeof(XmlIgnoreAttribute).FullName);
			codeMemberField.CustomAttributes.Add(value);
			codeClass.Members.Add(codeMemberField);
		}
	}

	private void ExportProperty(CodeTypeDeclaration codeClass, MemberMapping member, string ns, CodeIdentifiers memberScope, CodeConstructor ctor)
	{
		string text = memberScope.AddUnique(CodeExporter.MakeFieldName(member.Name), member);
		string typeName = member.GetTypeName(base.CodeProvider);
		CodeMemberField codeMemberField = new CodeMemberField(typeName, text);
		codeMemberField.Attributes = MemberAttributes.Private;
		codeClass.Members.Add(codeMemberField);
		CodeMemberProperty codeMemberProperty = CreatePropertyDeclaration(codeMemberField, member.Name, typeName);
		codeMemberProperty.Comments.Add(new CodeCommentStatement(Res.GetString("<remarks/>"), docComment: true));
		AddMemberMetadata(codeMemberField, codeMemberProperty.CustomAttributes, member, ns, forceUseMemberName: false, codeMemberProperty.Comments, ctor);
		codeClass.Members.Add(codeMemberProperty);
		if (member.CheckSpecified != 0)
		{
			codeMemberField = new CodeMemberField(typeof(bool).FullName, text + "Specified");
			codeMemberField.Attributes = MemberAttributes.Private;
			codeClass.Members.Add(codeMemberField);
			codeMemberProperty = CreatePropertyDeclaration(codeMemberField, member.Name + "Specified", typeof(bool).FullName);
			codeMemberProperty.Comments.Add(new CodeCommentStatement(Res.GetString("<remarks/>"), docComment: true));
			CodeAttributeDeclaration value = new CodeAttributeDeclaration(typeof(XmlIgnoreAttribute).FullName);
			codeMemberProperty.CustomAttributes.Add(value);
			codeClass.Members.Add(codeMemberProperty);
		}
	}

	private void ExportText(CodeAttributeDeclarationCollection metadata, TypeDesc typeDesc, string dataType)
	{
		CodeAttributeDeclaration codeAttributeDeclaration = new CodeAttributeDeclaration(typeof(XmlTextAttribute).FullName);
		if (typeDesc != null)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument(new CodeTypeOfExpression(typeDesc.FullName)));
		}
		if (dataType != null)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument("DataType", new CodePrimitiveExpression(dataType)));
		}
		metadata.Add(codeAttributeDeclaration);
	}

	private void ExportAttribute(CodeAttributeDeclarationCollection metadata, string name, string ns, TypeDesc typeDesc, TypeDesc dataTypeDesc, XmlSchemaForm form)
	{
		ExportMetadata(metadata, typeof(XmlAttributeAttribute), name, ns, typeDesc, dataTypeDesc, null, form, 0, -1);
	}

	private void ExportArrayItem(CodeAttributeDeclarationCollection metadata, string name, string ns, TypeDesc typeDesc, TypeDesc dataTypeDesc, bool isNullable, XmlSchemaForm form, int nestingLevel)
	{
		ExportMetadata(metadata, typeof(XmlArrayItemAttribute), name, ns, typeDesc, dataTypeDesc, isNullable ? null : ((object)false), form, nestingLevel, -1);
	}

	private void ExportElement(CodeAttributeDeclarationCollection metadata, string name, string ns, TypeDesc typeDesc, TypeDesc dataTypeDesc, bool isNullable, XmlSchemaForm form, int sequenceId)
	{
		ExportMetadata(metadata, typeof(XmlElementAttribute), name, ns, typeDesc, dataTypeDesc, isNullable ? ((object)true) : null, form, 0, sequenceId);
	}

	private void ExportArray(CodeAttributeDeclarationCollection metadata, string name, string ns, bool isNullable, XmlSchemaForm form, int sequenceId)
	{
		ExportMetadata(metadata, typeof(XmlArrayAttribute), name, ns, null, null, isNullable ? ((object)true) : null, form, 0, sequenceId);
	}

	private void ExportMetadata(CodeAttributeDeclarationCollection metadata, Type attributeType, string name, string ns, TypeDesc typeDesc, TypeDesc dataTypeDesc, object isNullable, XmlSchemaForm form, int nestingLevel, int sequenceId)
	{
		CodeAttributeDeclaration codeAttributeDeclaration = new CodeAttributeDeclaration(attributeType.FullName);
		if (name != null)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument(new CodePrimitiveExpression(name)));
		}
		if (typeDesc != null)
		{
			if (isNullable != null && (bool)isNullable && typeDesc.IsValueType && !typeDesc.IsMappedType && base.CodeProvider.Supports(GeneratorSupport.GenericTypeReference))
			{
				codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument(new CodeTypeOfExpression("System.Nullable`1[" + typeDesc.FullName + "]")));
				isNullable = null;
			}
			else
			{
				codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument(new CodeTypeOfExpression(typeDesc.FullName)));
			}
		}
		if (form != 0)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument("Form", new CodeFieldReferenceExpression(new CodeTypeReferenceExpression(typeof(XmlSchemaForm).FullName), Enum.Format(typeof(XmlSchemaForm), form, "G"))));
			if (form == XmlSchemaForm.Unqualified && ns != null && ns.Length == 0)
			{
				ns = null;
			}
		}
		if (ns != null)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument("Namespace", new CodePrimitiveExpression(ns)));
		}
		if (dataTypeDesc != null && dataTypeDesc.IsAmbiguousDataType && !dataTypeDesc.IsMappedType)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument("DataType", new CodePrimitiveExpression(dataTypeDesc.DataType.Name)));
		}
		if (isNullable != null)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument("IsNullable", new CodePrimitiveExpression((bool)isNullable)));
		}
		if (nestingLevel > 0)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument("NestingLevel", new CodePrimitiveExpression(nestingLevel)));
		}
		if (sequenceId >= 0)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument("Order", new CodePrimitiveExpression(sequenceId)));
		}
		if (codeAttributeDeclaration.Arguments.Count != 0 || !(attributeType == typeof(XmlElementAttribute)))
		{
			metadata.Add(codeAttributeDeclaration);
		}
	}

	private void ExportAnyElement(CodeAttributeDeclarationCollection metadata, string name, string ns, int sequenceId)
	{
		CodeAttributeDeclaration codeAttributeDeclaration = new CodeAttributeDeclaration(typeof(XmlAnyElementAttribute).FullName);
		if (name != null && name.Length > 0)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument("Name", new CodePrimitiveExpression(name)));
		}
		if (ns != null)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument("Namespace", new CodePrimitiveExpression(ns)));
		}
		if (sequenceId >= 0)
		{
			codeAttributeDeclaration.Arguments.Add(new CodeAttributeArgument("Order", new CodePrimitiveExpression(sequenceId)));
		}
		metadata.Add(codeAttributeDeclaration);
	}

	private void ExportAnyAttribute(CodeAttributeDeclarationCollection metadata)
	{
		metadata.Add(new CodeAttributeDeclaration(typeof(XmlAnyAttributeAttribute).FullName));
	}

	internal override void EnsureTypesExported(Accessor[] accessors, string ns)
	{
		if (accessors != null)
		{
			for (int i = 0; i < accessors.Length; i++)
			{
				EnsureTypesExported(accessors[i], ns);
			}
		}
	}

	private void EnsureTypesExported(Accessor accessor, string ns)
	{
		if (accessor != null)
		{
			ExportType(accessor.Mapping, null, ns, null, checkReference: false);
		}
	}
}

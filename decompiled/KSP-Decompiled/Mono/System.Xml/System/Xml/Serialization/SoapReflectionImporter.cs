using System.Collections;
using System.Globalization;
using System.Reflection;
using System.Threading;
using System.Xml.Schema;

namespace System.Xml.Serialization;

public class SoapReflectionImporter
{
	private TypeScope typeScope;

	private SoapAttributeOverrides attributeOverrides;

	private NameTable types = new NameTable();

	private NameTable nullables = new NameTable();

	private StructMapping root;

	private string defaultNs;

	private ModelScope modelScope;

	public SoapReflectionImporter()
		: this(null, null)
	{
	}

	public SoapReflectionImporter(string defaultNamespace)
		: this(null, defaultNamespace)
	{
	}

	public SoapReflectionImporter(SoapAttributeOverrides attributeOverrides)
		: this(attributeOverrides, null)
	{
	}

	public SoapReflectionImporter(SoapAttributeOverrides attributeOverrides, string defaultNamespace)
	{
		if (defaultNamespace == null)
		{
			defaultNamespace = string.Empty;
		}
		if (attributeOverrides == null)
		{
			attributeOverrides = new SoapAttributeOverrides();
		}
		this.attributeOverrides = attributeOverrides;
		defaultNs = defaultNamespace;
		typeScope = new TypeScope();
		modelScope = new ModelScope(typeScope);
	}

	public void IncludeTypes(ICustomAttributeProvider provider)
	{
		IncludeTypes(provider, new RecursionLimiter());
	}

	private void IncludeTypes(ICustomAttributeProvider provider, RecursionLimiter limiter)
	{
		object[] customAttributes = provider.GetCustomAttributes(typeof(SoapIncludeAttribute), inherit: false);
		for (int i = 0; i < customAttributes.Length; i++)
		{
			IncludeType(((SoapIncludeAttribute)customAttributes[i]).Type, limiter);
		}
	}

	public void IncludeType(Type type)
	{
		IncludeType(type, new RecursionLimiter());
	}

	private void IncludeType(Type type, RecursionLimiter limiter)
	{
		ImportTypeMapping(modelScope.GetTypeModel(type), limiter);
	}

	public XmlTypeMapping ImportTypeMapping(Type type)
	{
		return ImportTypeMapping(type, null);
	}

	public XmlTypeMapping ImportTypeMapping(Type type, string defaultNamespace)
	{
		ElementAccessor elementAccessor = new ElementAccessor();
		elementAccessor.IsSoap = true;
		elementAccessor.Mapping = ImportTypeMapping(modelScope.GetTypeModel(type), new RecursionLimiter());
		elementAccessor.Name = elementAccessor.Mapping.DefaultElementName;
		elementAccessor.Namespace = ((elementAccessor.Mapping.Namespace == null) ? defaultNamespace : elementAccessor.Mapping.Namespace);
		elementAccessor.Form = XmlSchemaForm.Qualified;
		XmlTypeMapping xmlTypeMapping = new XmlTypeMapping(typeScope, elementAccessor);
		xmlTypeMapping.SetKeyInternal(XmlMapping.GenerateKey(type, null, defaultNamespace));
		xmlTypeMapping.IsSoap = true;
		xmlTypeMapping.GenerateSerializer = true;
		return xmlTypeMapping;
	}

	public XmlMembersMapping ImportMembersMapping(string elementName, string ns, XmlReflectionMember[] members)
	{
		return ImportMembersMapping(elementName, ns, members, hasWrapperElement: true, writeAccessors: true, validate: false);
	}

	public XmlMembersMapping ImportMembersMapping(string elementName, string ns, XmlReflectionMember[] members, bool hasWrapperElement, bool writeAccessors)
	{
		return ImportMembersMapping(elementName, ns, members, hasWrapperElement, writeAccessors, validate: false);
	}

	public XmlMembersMapping ImportMembersMapping(string elementName, string ns, XmlReflectionMember[] members, bool hasWrapperElement, bool writeAccessors, bool validate)
	{
		return ImportMembersMapping(elementName, ns, members, hasWrapperElement, writeAccessors, validate, XmlMappingAccess.Read | XmlMappingAccess.Write);
	}

	public XmlMembersMapping ImportMembersMapping(string elementName, string ns, XmlReflectionMember[] members, bool hasWrapperElement, bool writeAccessors, bool validate, XmlMappingAccess access)
	{
		ElementAccessor elementAccessor = new ElementAccessor();
		elementAccessor.IsSoap = true;
		elementAccessor.Name = ((elementName == null || elementName.Length == 0) ? elementName : XmlConvert.EncodeLocalName(elementName));
		elementAccessor.Mapping = ImportMembersMapping(members, ns, hasWrapperElement, writeAccessors, validate, new RecursionLimiter());
		elementAccessor.Mapping.TypeName = elementName;
		elementAccessor.Namespace = ((elementAccessor.Mapping.Namespace == null) ? ns : elementAccessor.Mapping.Namespace);
		elementAccessor.Form = XmlSchemaForm.Qualified;
		return new XmlMembersMapping(typeScope, elementAccessor, access)
		{
			IsSoap = true,
			GenerateSerializer = true
		};
	}

	private Exception ReflectionException(string context, Exception e)
	{
		return new InvalidOperationException(Res.GetString("There was an error reflecting '{0}'.", context), e);
	}

	private SoapAttributes GetAttributes(Type type)
	{
		SoapAttributes soapAttributes = attributeOverrides[type];
		if (soapAttributes != null)
		{
			return soapAttributes;
		}
		return new SoapAttributes(type);
	}

	private SoapAttributes GetAttributes(MemberInfo memberInfo)
	{
		SoapAttributes soapAttributes = attributeOverrides[memberInfo.DeclaringType, memberInfo.Name];
		if (soapAttributes != null)
		{
			return soapAttributes;
		}
		return new SoapAttributes(memberInfo);
	}

	private TypeMapping ImportTypeMapping(TypeModel model, RecursionLimiter limiter)
	{
		return ImportTypeMapping(model, string.Empty, limiter);
	}

	private TypeMapping ImportTypeMapping(TypeModel model, string dataType, RecursionLimiter limiter)
	{
		if (dataType.Length > 0)
		{
			if (!model.TypeDesc.IsPrimitive)
			{
				throw new InvalidOperationException(Res.GetString("'{0}' is an invalid value for the {1} property. The property may only be specified for primitive types.", dataType, "SoapElementAttribute.DataType"));
			}
			TypeDesc typeDesc = typeScope.GetTypeDesc(dataType, "http://www.w3.org/2001/XMLSchema");
			if (typeDesc == null)
			{
				throw new InvalidOperationException(Res.GetString("Value '{0}' cannot be used for the {1} property. The datatype '{2}' is missing.", dataType, "SoapElementAttribute.DataType", new XmlQualifiedName(dataType, "http://www.w3.org/2001/XMLSchema").ToString()));
			}
			if (model.TypeDesc.FullName != typeDesc.FullName)
			{
				throw new InvalidOperationException(Res.GetString("'{0}' is an invalid value for the {1} property. {0} cannot be converted to {2}.", dataType, "SoapElementAttribute.DataType", model.TypeDesc.FullName));
			}
		}
		if (((uint)GetAttributes(model.Type).SoapFlags & 0xFFFFFFFDu) != 0)
		{
			throw new InvalidOperationException(Res.GetString("XmlRoot and XmlType attributes may not be specified for the type {0}.", model.Type.FullName));
		}
		switch (model.TypeDesc.Kind)
		{
		case TypeKind.Enum:
			return ImportEnumMapping((EnumModel)model);
		case TypeKind.Primitive:
			return ImportPrimitiveMapping((PrimitiveModel)model, dataType);
		case TypeKind.Array:
		case TypeKind.Collection:
		case TypeKind.Enumerable:
			return ImportArrayLikeMapping((ArrayModel)model, limiter);
		case TypeKind.Root:
		case TypeKind.Struct:
		case TypeKind.Class:
			if (model.TypeDesc.IsOptionalValue)
			{
				TypeDesc baseTypeDesc = model.TypeDesc.BaseTypeDesc;
				SoapAttributes attributes = GetAttributes(baseTypeDesc.Type);
				string @namespace = defaultNs;
				if (attributes.SoapType != null && attributes.SoapType.Namespace != null)
				{
					@namespace = attributes.SoapType.Namespace;
				}
				TypeDesc typeDesc2 = (string.IsNullOrEmpty(dataType) ? model.TypeDesc.BaseTypeDesc : typeScope.GetTypeDesc(dataType, "http://www.w3.org/2001/XMLSchema"));
				string typeName = (string.IsNullOrEmpty(dataType) ? model.TypeDesc.BaseTypeDesc.Name : dataType);
				TypeMapping typeMapping = GetTypeMapping(typeName, @namespace, typeDesc2);
				if (typeMapping == null)
				{
					typeMapping = ImportTypeMapping(modelScope.GetTypeModel(baseTypeDesc.Type), dataType, limiter);
				}
				return CreateNullableMapping(typeMapping, model.TypeDesc.Type);
			}
			return ImportStructLikeMapping((StructModel)model, limiter);
		default:
			throw new NotSupportedException(Res.GetString("The type {0} may not be serialized with SOAP-encoded messages. Set the Use for your message to Literal.", model.TypeDesc.FullName));
		}
	}

	private StructMapping CreateRootMapping()
	{
		TypeDesc typeDesc = typeScope.GetTypeDesc(typeof(object));
		return new StructMapping
		{
			IsSoap = true,
			TypeDesc = typeDesc,
			Members = new MemberMapping[0],
			IncludeInSchema = false,
			TypeName = "anyType",
			Namespace = "http://www.w3.org/2001/XMLSchema"
		};
	}

	private StructMapping GetRootMapping()
	{
		if (root == null)
		{
			root = CreateRootMapping();
			typeScope.AddTypeMapping(root);
		}
		return root;
	}

	private TypeMapping GetTypeMapping(string typeName, string ns, TypeDesc typeDesc)
	{
		TypeMapping typeMapping = (TypeMapping)types[typeName, ns];
		if (typeMapping == null)
		{
			return null;
		}
		if (typeMapping.TypeDesc != typeDesc)
		{
			throw new InvalidOperationException(Res.GetString("Types '{0}' and '{1}' both use the XML type name, '{2}', from namespace '{3}'. Use XML attributes to specify a unique XML name and/or namespace for the type.", typeDesc.FullName, typeMapping.TypeDesc.FullName, typeName, ns));
		}
		return typeMapping;
	}

	private NullableMapping CreateNullableMapping(TypeMapping baseMapping, Type type)
	{
		TypeDesc nullableTypeDesc = baseMapping.TypeDesc.GetNullableTypeDesc(type);
		TypeMapping typeMapping = (TypeMapping)nullables[baseMapping.TypeName, baseMapping.Namespace];
		NullableMapping nullableMapping;
		if (typeMapping != null)
		{
			if (typeMapping is NullableMapping)
			{
				nullableMapping = (NullableMapping)typeMapping;
				if (nullableMapping.BaseMapping is PrimitiveMapping && baseMapping is PrimitiveMapping)
				{
					return nullableMapping;
				}
				if (nullableMapping.BaseMapping == baseMapping)
				{
					return nullableMapping;
				}
				throw new InvalidOperationException(Res.GetString("Types '{0}' and '{1}' both use the XML type name, '{2}', from namespace '{3}'. Use XML attributes to specify a unique XML name and/or namespace for the type.", nullableTypeDesc.FullName, typeMapping.TypeDesc.FullName, nullableTypeDesc.Name, typeMapping.Namespace));
			}
			if (!(baseMapping is PrimitiveMapping))
			{
				throw new InvalidOperationException(Res.GetString("Types '{0}' and '{1}' both use the XML type name, '{2}', from namespace '{3}'. Use XML attributes to specify a unique XML name and/or namespace for the type.", nullableTypeDesc.FullName, typeMapping.TypeDesc.FullName, nullableTypeDesc.Name, typeMapping.Namespace));
			}
		}
		nullableMapping = new NullableMapping();
		nullableMapping.BaseMapping = baseMapping;
		nullableMapping.TypeDesc = nullableTypeDesc;
		nullableMapping.TypeName = baseMapping.TypeName;
		nullableMapping.Namespace = baseMapping.Namespace;
		nullableMapping.IncludeInSchema = false;
		nullables.Add(baseMapping.TypeName, nullableMapping.Namespace, nullableMapping);
		typeScope.AddTypeMapping(nullableMapping);
		return nullableMapping;
	}

	private StructMapping ImportStructLikeMapping(StructModel model, RecursionLimiter limiter)
	{
		if (model.TypeDesc.Kind == TypeKind.Root)
		{
			return GetRootMapping();
		}
		SoapAttributes attributes = GetAttributes(model.Type);
		string @namespace = defaultNs;
		if (attributes.SoapType != null && attributes.SoapType.Namespace != null)
		{
			@namespace = attributes.SoapType.Namespace;
		}
		string name = XsdTypeName(model.Type, attributes, model.TypeDesc.Name);
		name = XmlConvert.EncodeLocalName(name);
		StructMapping structMapping = (StructMapping)GetTypeMapping(name, @namespace, model.TypeDesc);
		if (structMapping == null)
		{
			structMapping = new StructMapping();
			structMapping.IsSoap = true;
			structMapping.TypeDesc = model.TypeDesc;
			structMapping.Namespace = @namespace;
			structMapping.TypeName = name;
			if (attributes.SoapType != null)
			{
				structMapping.IncludeInSchema = attributes.SoapType.IncludeInSchema;
			}
			typeScope.AddTypeMapping(structMapping);
			types.Add(name, @namespace, structMapping);
			if (limiter.IsExceededLimit)
			{
				limiter.DeferredWorkItems.Add(new ImportStructWorkItem(model, structMapping));
				return structMapping;
			}
			limiter.Depth++;
			InitializeStructMembers(structMapping, model, limiter);
			while (limiter.DeferredWorkItems.Count > 0)
			{
				int index = limiter.DeferredWorkItems.Count - 1;
				ImportStructWorkItem importStructWorkItem = limiter.DeferredWorkItems[index];
				if (InitializeStructMembers(importStructWorkItem.Mapping, importStructWorkItem.Model, limiter))
				{
					limiter.DeferredWorkItems.RemoveAt(index);
				}
			}
			limiter.Depth--;
		}
		return structMapping;
	}

	private bool InitializeStructMembers(StructMapping mapping, StructModel model, RecursionLimiter limiter)
	{
		if (mapping.IsFullyInitialized)
		{
			return true;
		}
		if (model.TypeDesc.BaseTypeDesc != null)
		{
			StructMapping baseMapping = ImportStructLikeMapping((StructModel)modelScope.GetTypeModel(model.Type.BaseType, directReference: false), limiter);
			int num = limiter.DeferredWorkItems.IndexOf(mapping.BaseMapping);
			if (num >= 0)
			{
				if (!limiter.DeferredWorkItems.Contains(mapping))
				{
					limiter.DeferredWorkItems.Add(new ImportStructWorkItem(model, mapping));
				}
				int num2 = limiter.DeferredWorkItems.Count - 1;
				if (num < num2)
				{
					ImportStructWorkItem value = limiter.DeferredWorkItems[num];
					limiter.DeferredWorkItems[num] = limiter.DeferredWorkItems[num2];
					limiter.DeferredWorkItems[num2] = value;
				}
				return false;
			}
			mapping.BaseMapping = baseMapping;
		}
		ArrayList arrayList = new ArrayList();
		MemberInfo[] memberInfos = model.GetMemberInfos();
		foreach (MemberInfo memberInfo in memberInfos)
		{
			if ((memberInfo.MemberType & (MemberTypes.Field | MemberTypes.Property)) == 0)
			{
				continue;
			}
			SoapAttributes attributes = GetAttributes(memberInfo);
			if (attributes.SoapIgnore)
			{
				continue;
			}
			FieldModel fieldModel = model.GetFieldModel(memberInfo);
			if (fieldModel == null)
			{
				continue;
			}
			MemberMapping memberMapping = ImportFieldMapping(fieldModel, attributes, mapping.Namespace, limiter);
			if (memberMapping == null)
			{
				continue;
			}
			if (!memberMapping.TypeDesc.IsPrimitive && !memberMapping.TypeDesc.IsEnum && !memberMapping.TypeDesc.IsOptionalValue)
			{
				if (model.TypeDesc.IsValueType)
				{
					throw new NotSupportedException(Res.GetString("Cannot serialize {0}. References in structs are not supported with encoded SOAP.", model.TypeDesc.FullName));
				}
				if (memberMapping.TypeDesc.IsValueType)
				{
					throw new NotSupportedException(Res.GetString("Cannot serialize {0}. Nested structs are not supported with encoded SOAP.", memberMapping.TypeDesc.FullName));
				}
			}
			if (mapping.BaseMapping == null || !mapping.BaseMapping.Declares(memberMapping, mapping.TypeName))
			{
				arrayList.Add(memberMapping);
			}
		}
		mapping.Members = (MemberMapping[])arrayList.ToArray(typeof(MemberMapping));
		if (mapping.BaseMapping == null)
		{
			mapping.BaseMapping = GetRootMapping();
		}
		IncludeTypes(model.Type, limiter);
		return true;
	}

	private ArrayMapping ImportArrayLikeMapping(ArrayModel model, RecursionLimiter limiter)
	{
		ArrayMapping arrayMapping = new ArrayMapping();
		arrayMapping.IsSoap = true;
		TypeMapping typeMapping = ImportTypeMapping(model.Element, limiter);
		if (typeMapping.TypeDesc.IsValueType && !typeMapping.TypeDesc.IsPrimitive && !typeMapping.TypeDesc.IsEnum)
		{
			throw new NotSupportedException(Res.GetString("Cannot serialize {0}. Arrays of structs are not supported with encoded SOAP.", model.TypeDesc.FullName));
		}
		arrayMapping.TypeDesc = model.TypeDesc;
		arrayMapping.Elements = new ElementAccessor[1] { CreateElementAccessor(typeMapping, arrayMapping.Namespace) };
		SetArrayMappingType(arrayMapping);
		ArrayMapping arrayMapping2 = (ArrayMapping)types[arrayMapping.TypeName, arrayMapping.Namespace];
		if (arrayMapping2 != null)
		{
			ArrayMapping next = arrayMapping2;
			while (arrayMapping2 != null)
			{
				if (arrayMapping2.TypeDesc == model.TypeDesc)
				{
					return arrayMapping2;
				}
				arrayMapping2 = arrayMapping2.Next;
			}
			arrayMapping.Next = next;
			types[arrayMapping.TypeName, arrayMapping.Namespace] = arrayMapping;
			return arrayMapping;
		}
		typeScope.AddTypeMapping(arrayMapping);
		types.Add(arrayMapping.TypeName, arrayMapping.Namespace, arrayMapping);
		IncludeTypes(model.Type);
		return arrayMapping;
	}

	private void SetArrayMappingType(ArrayMapping mapping)
	{
		bool flag = false;
		TypeMapping typeMapping = ((mapping.Elements.Length != 1) ? null : mapping.Elements[0].Mapping);
		string text;
		string identifier;
		if (typeMapping is EnumMapping)
		{
			text = typeMapping.Namespace;
			identifier = typeMapping.TypeName;
		}
		else if (typeMapping is PrimitiveMapping)
		{
			text = (typeMapping.TypeDesc.IsXsdType ? "http://www.w3.org/2001/XMLSchema" : "http://microsoft.com/wsdl/types/");
			identifier = typeMapping.TypeDesc.DataType.Name;
			flag = true;
		}
		else if (typeMapping is StructMapping)
		{
			if (typeMapping.TypeDesc.IsRoot)
			{
				text = "http://www.w3.org/2001/XMLSchema";
				identifier = "anyType";
				flag = true;
			}
			else
			{
				text = typeMapping.Namespace;
				identifier = typeMapping.TypeName;
			}
		}
		else
		{
			if (!(typeMapping is ArrayMapping))
			{
				throw new InvalidOperationException(Res.GetString("An array of type {0} may not be used with XmlArrayType.Soap.", mapping.TypeDesc.FullName));
			}
			text = typeMapping.Namespace;
			identifier = typeMapping.TypeName;
		}
		identifier = CodeIdentifier.MakePascal(identifier);
		string text2 = "ArrayOf" + identifier;
		string text3 = (flag ? defaultNs : text);
		int num = 1;
		TypeMapping typeMapping2 = (TypeMapping)types[text2, text3];
		while (typeMapping2 != null && (!(typeMapping2 is ArrayMapping) || !AccessorMapping.ElementsMatch(((ArrayMapping)typeMapping2).Elements, mapping.Elements)))
		{
			text2 = identifier + num.ToString(CultureInfo.InvariantCulture);
			typeMapping2 = (TypeMapping)types[text2, text3];
			num++;
		}
		mapping.Namespace = text3;
		mapping.TypeName = text2;
	}

	private PrimitiveMapping ImportPrimitiveMapping(PrimitiveModel model, string dataType)
	{
		PrimitiveMapping primitiveMapping = new PrimitiveMapping();
		primitiveMapping.IsSoap = true;
		if (dataType.Length > 0)
		{
			primitiveMapping.TypeDesc = typeScope.GetTypeDesc(dataType, "http://www.w3.org/2001/XMLSchema");
			if (primitiveMapping.TypeDesc == null)
			{
				primitiveMapping.TypeDesc = typeScope.GetTypeDesc(dataType, "http://microsoft.com/wsdl/types/");
				if (primitiveMapping.TypeDesc == null)
				{
					throw new InvalidOperationException(Res.GetString("The type, {0}, is undeclared.", dataType));
				}
			}
		}
		else
		{
			primitiveMapping.TypeDesc = model.TypeDesc;
		}
		primitiveMapping.TypeName = primitiveMapping.TypeDesc.DataType.Name;
		primitiveMapping.Namespace = (primitiveMapping.TypeDesc.IsXsdType ? "http://www.w3.org/2001/XMLSchema" : "http://microsoft.com/wsdl/types/");
		return primitiveMapping;
	}

	private EnumMapping ImportEnumMapping(EnumModel model)
	{
		SoapAttributes attributes = GetAttributes(model.Type);
		string @namespace = defaultNs;
		if (attributes.SoapType != null && attributes.SoapType.Namespace != null)
		{
			@namespace = attributes.SoapType.Namespace;
		}
		string name = XsdTypeName(model.Type, attributes, model.TypeDesc.Name);
		name = XmlConvert.EncodeLocalName(name);
		EnumMapping enumMapping = (EnumMapping)GetTypeMapping(name, @namespace, model.TypeDesc);
		if (enumMapping == null)
		{
			enumMapping = new EnumMapping();
			enumMapping.IsSoap = true;
			enumMapping.TypeDesc = model.TypeDesc;
			enumMapping.TypeName = name;
			enumMapping.Namespace = @namespace;
			enumMapping.IsFlags = model.Type.IsDefined(typeof(FlagsAttribute), inherit: false);
			typeScope.AddTypeMapping(enumMapping);
			types.Add(name, @namespace, enumMapping);
			ArrayList arrayList = new ArrayList();
			for (int i = 0; i < model.Constants.Length; i++)
			{
				ConstantMapping constantMapping = ImportConstantMapping(model.Constants[i]);
				if (constantMapping != null)
				{
					arrayList.Add(constantMapping);
				}
			}
			if (arrayList.Count == 0)
			{
				throw new InvalidOperationException(Res.GetString("Cannot serialize object of type '{0}'. The object does not have serializable members.", model.TypeDesc.FullName));
			}
			enumMapping.Constants = (ConstantMapping[])arrayList.ToArray(typeof(ConstantMapping));
		}
		return enumMapping;
	}

	private ConstantMapping ImportConstantMapping(ConstantModel model)
	{
		SoapAttributes attributes = GetAttributes(model.FieldInfo);
		if (attributes.SoapIgnore)
		{
			return null;
		}
		if (((uint)attributes.SoapFlags & 0xFFFFFFFEu) != 0)
		{
			throw new InvalidOperationException(Res.GetString("Only SoapEnum may be used on enum constants."));
		}
		if (attributes.SoapEnum == null)
		{
			attributes.SoapEnum = new SoapEnumAttribute();
		}
		return new ConstantMapping
		{
			XmlName = ((attributes.SoapEnum.Name.Length == 0) ? model.Name : attributes.SoapEnum.Name),
			Name = model.Name,
			Value = model.Value
		};
	}

	private MembersMapping ImportMembersMapping(XmlReflectionMember[] xmlReflectionMembers, string ns, bool hasWrapperElement, bool writeAccessors, bool validateWrapperElement, RecursionLimiter limiter)
	{
		MembersMapping membersMapping = new MembersMapping();
		membersMapping.TypeDesc = typeScope.GetTypeDesc(typeof(object[]));
		MemberMapping[] array = new MemberMapping[xmlReflectionMembers.Length];
		for (int i = 0; i < array.Length; i++)
		{
			try
			{
				XmlReflectionMember xmlReflectionMember = xmlReflectionMembers[i];
				MemberMapping memberMapping = ImportMemberMapping(xmlReflectionMember, ns, xmlReflectionMembers, (!hasWrapperElement) ? XmlSchemaForm.Qualified : XmlSchemaForm.Unqualified, limiter);
				if (xmlReflectionMember.IsReturnValue && writeAccessors)
				{
					if (i > 0)
					{
						throw new InvalidOperationException(Res.GetString("The return value must be the first member."));
					}
					memberMapping.IsReturnValue = true;
				}
				array[i] = memberMapping;
			}
			catch (Exception ex)
			{
				if (ex is ThreadAbortException || ex is StackOverflowException || ex is OutOfMemoryException)
				{
					throw;
				}
				throw ReflectionException(xmlReflectionMembers[i].MemberName, ex);
			}
		}
		membersMapping.Members = array;
		membersMapping.HasWrapperElement = hasWrapperElement;
		if (hasWrapperElement)
		{
			membersMapping.ValidateRpcWrapperElement = validateWrapperElement;
		}
		membersMapping.WriteAccessors = writeAccessors;
		membersMapping.IsSoap = true;
		if (hasWrapperElement && !writeAccessors)
		{
			membersMapping.Namespace = ns;
		}
		return membersMapping;
	}

	private MemberMapping ImportMemberMapping(XmlReflectionMember xmlReflectionMember, string ns, XmlReflectionMember[] xmlReflectionMembers, XmlSchemaForm form, RecursionLimiter limiter)
	{
		SoapAttributes soapAttributes = xmlReflectionMember.SoapAttributes;
		if (soapAttributes.SoapIgnore)
		{
			return null;
		}
		MemberMapping memberMapping = new MemberMapping();
		memberMapping.IsSoap = true;
		memberMapping.Name = xmlReflectionMember.MemberName;
		bool checkSpecified = XmlReflectionImporter.FindSpecifiedMember(xmlReflectionMember.MemberName, xmlReflectionMembers) != null;
		FieldModel fieldModel = new FieldModel(xmlReflectionMember.MemberName, xmlReflectionMember.MemberType, typeScope.GetTypeDesc(xmlReflectionMember.MemberType), checkSpecified, checkShouldPersist: false);
		memberMapping.CheckShouldPersist = fieldModel.CheckShouldPersist;
		memberMapping.CheckSpecified = fieldModel.CheckSpecified;
		memberMapping.ReadOnly = fieldModel.ReadOnly;
		ImportAccessorMapping(memberMapping, fieldModel, soapAttributes, ns, form, limiter);
		if (xmlReflectionMember.OverrideIsNullable)
		{
			memberMapping.Elements[0].IsNullable = false;
		}
		return memberMapping;
	}

	private MemberMapping ImportFieldMapping(FieldModel model, SoapAttributes a, string ns, RecursionLimiter limiter)
	{
		if (a.SoapIgnore)
		{
			return null;
		}
		MemberMapping memberMapping = new MemberMapping();
		memberMapping.IsSoap = true;
		memberMapping.Name = model.Name;
		memberMapping.CheckShouldPersist = model.CheckShouldPersist;
		memberMapping.CheckSpecified = model.CheckSpecified;
		memberMapping.MemberInfo = model.MemberInfo;
		memberMapping.CheckSpecifiedMemberInfo = model.CheckSpecifiedMemberInfo;
		memberMapping.CheckShouldPersistMethodInfo = model.CheckShouldPersistMethodInfo;
		memberMapping.ReadOnly = model.ReadOnly;
		ImportAccessorMapping(memberMapping, model, a, ns, XmlSchemaForm.Unqualified, limiter);
		return memberMapping;
	}

	private void ImportAccessorMapping(MemberMapping accessor, FieldModel model, SoapAttributes a, string ns, XmlSchemaForm form, RecursionLimiter limiter)
	{
		Type fieldType = model.FieldType;
		string name = model.Name;
		accessor.TypeDesc = typeScope.GetTypeDesc(fieldType);
		if (accessor.TypeDesc.IsVoid)
		{
			throw new InvalidOperationException(Res.GetString("The type Void is not valid in this context."));
		}
		SoapAttributeFlags soapFlags = a.SoapFlags;
		if ((soapFlags & SoapAttributeFlags.Attribute) == SoapAttributeFlags.Attribute)
		{
			if (!accessor.TypeDesc.IsPrimitive && !accessor.TypeDesc.IsEnum)
			{
				throw new InvalidOperationException(Res.GetString("Cannot serialize member '{0}' of type {1}. SoapAttribute cannot be used to encode complex types.", name, accessor.TypeDesc.FullName));
			}
			if ((soapFlags & SoapAttributeFlags.Attribute) != soapFlags)
			{
				throw new InvalidOperationException(Res.GetString("Only SoapElementAttribute or SoapAttributeAttribute may be used on members."));
			}
			AttributeAccessor attributeAccessor = new AttributeAccessor();
			attributeAccessor.Name = Accessor.EscapeQName((a.SoapAttribute == null || a.SoapAttribute.AttributeName.Length == 0) ? name : a.SoapAttribute.AttributeName);
			attributeAccessor.Namespace = ((a.SoapAttribute == null || a.SoapAttribute.Namespace == null) ? ns : a.SoapAttribute.Namespace);
			attributeAccessor.Form = XmlSchemaForm.Qualified;
			attributeAccessor.Mapping = ImportTypeMapping(modelScope.GetTypeModel(fieldType), (a.SoapAttribute == null) ? string.Empty : a.SoapAttribute.DataType, limiter);
			attributeAccessor.Default = GetDefaultValue(model.FieldTypeDesc, a);
			accessor.Attribute = attributeAccessor;
			accessor.Elements = new ElementAccessor[0];
		}
		else
		{
			if ((soapFlags & SoapAttributeFlags.Element) != soapFlags)
			{
				throw new InvalidOperationException(Res.GetString("Only SoapElementAttribute or SoapAttributeAttribute may be used on members."));
			}
			ElementAccessor elementAccessor = new ElementAccessor();
			elementAccessor.IsSoap = true;
			elementAccessor.Name = XmlConvert.EncodeLocalName((a.SoapElement == null || a.SoapElement.ElementName.Length == 0) ? name : a.SoapElement.ElementName);
			elementAccessor.Namespace = ns;
			elementAccessor.Form = form;
			elementAccessor.Mapping = ImportTypeMapping(modelScope.GetTypeModel(fieldType), (a.SoapElement == null) ? string.Empty : a.SoapElement.DataType, limiter);
			if (a.SoapElement != null)
			{
				elementAccessor.IsNullable = a.SoapElement.IsNullable;
			}
			accessor.Elements = new ElementAccessor[1] { elementAccessor };
		}
	}

	private static ElementAccessor CreateElementAccessor(TypeMapping mapping, string ns)
	{
		return new ElementAccessor
		{
			IsSoap = true,
			Name = mapping.TypeName,
			Namespace = ns,
			Mapping = mapping
		};
	}

	private object GetDefaultValue(TypeDesc fieldTypeDesc, SoapAttributes a)
	{
		if (a.SoapDefaultValue == null || a.SoapDefaultValue == DBNull.Value)
		{
			return null;
		}
		if (fieldTypeDesc.Kind != TypeKind.Primitive && fieldTypeDesc.Kind != TypeKind.Enum)
		{
			a.SoapDefaultValue = null;
			return a.SoapDefaultValue;
		}
		if (fieldTypeDesc.Kind == TypeKind.Enum)
		{
			if (fieldTypeDesc != typeScope.GetTypeDesc(a.SoapDefaultValue.GetType()))
			{
				throw new InvalidOperationException(Res.GetString("Enum {0} cannot be converted to {1}.", a.SoapDefaultValue.GetType().FullName, fieldTypeDesc.FullName));
			}
			string text = Enum.Format(a.SoapDefaultValue.GetType(), a.SoapDefaultValue, "G").Replace(",", " ");
			string text2 = Enum.Format(a.SoapDefaultValue.GetType(), a.SoapDefaultValue, "D");
			if (text == text2)
			{
				throw new InvalidOperationException(Res.GetString("Value '{0}' cannot be converted to {1}.", text, a.SoapDefaultValue.GetType().FullName));
			}
			return text;
		}
		return a.SoapDefaultValue;
	}

	internal string XsdTypeName(Type type)
	{
		if (type == typeof(object))
		{
			return "anyType";
		}
		TypeDesc typeDesc = typeScope.GetTypeDesc(type);
		if (typeDesc.IsPrimitive && typeDesc.DataType != null && typeDesc.DataType.Name != null && typeDesc.DataType.Name.Length > 0)
		{
			return typeDesc.DataType.Name;
		}
		return XsdTypeName(type, GetAttributes(type), typeDesc.Name);
	}

	internal string XsdTypeName(Type type, SoapAttributes a, string name)
	{
		string text = name;
		if (a.SoapType != null && a.SoapType.TypeName.Length > 0)
		{
			text = a.SoapType.TypeName;
		}
		if (type.IsGenericType && text.IndexOf('{') >= 0)
		{
			Type[] genericArguments = type.GetGenericTypeDefinition().GetGenericArguments();
			Type[] genericArguments2 = type.GetGenericArguments();
			for (int i = 0; i < genericArguments.Length; i++)
			{
				string text2 = string.Concat("{", genericArguments[i], "}");
				if (text.Contains(text2))
				{
					text = text.Replace(text2, XsdTypeName(genericArguments2[i]));
					if (text.IndexOf('{') < 0)
					{
						break;
					}
				}
			}
		}
		return text;
	}
}

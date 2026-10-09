using System.Collections;
using System.Globalization;
using System.Reflection;
using System.Xml.Schema;

namespace System.Xml.Serialization;

public class XmlSchemaExporter
{
	internal const XmlSchemaForm elementFormDefault = XmlSchemaForm.Qualified;

	internal const XmlSchemaForm attributeFormDefault = XmlSchemaForm.Unqualified;

	private XmlSchemas schemas;

	private Hashtable elements = new Hashtable();

	private Hashtable attributes = new Hashtable();

	private Hashtable types = new Hashtable();

	private Hashtable references = new Hashtable();

	private bool needToExportRoot;

	private TypeScope scope;

	public XmlSchemaExporter(XmlSchemas schemas)
	{
		this.schemas = schemas;
	}

	public void ExportTypeMapping(XmlTypeMapping xmlTypeMapping)
	{
		xmlTypeMapping.CheckShallow();
		CheckScope(xmlTypeMapping.Scope);
		ExportElement(xmlTypeMapping.Accessor);
		ExportRootIfNecessary(xmlTypeMapping.Scope);
	}

	public XmlQualifiedName ExportTypeMapping(XmlMembersMapping xmlMembersMapping)
	{
		xmlMembersMapping.CheckShallow();
		CheckScope(xmlMembersMapping.Scope);
		MembersMapping membersMapping = (MembersMapping)xmlMembersMapping.Accessor.Mapping;
		if (membersMapping.Members.Length == 1 && membersMapping.Members[0].Elements[0].Mapping is SpecialMapping)
		{
			SpecialMapping mapping = (SpecialMapping)membersMapping.Members[0].Elements[0].Mapping;
			XmlSchemaType xmlSchemaType = ExportSpecialMapping(mapping, xmlMembersMapping.Accessor.Namespace, isAny: false, null);
			if (xmlSchemaType != null && xmlSchemaType.Name != null && xmlSchemaType.Name.Length > 0)
			{
				xmlSchemaType.Name = xmlMembersMapping.Accessor.Name;
				AddSchemaItem(xmlSchemaType, xmlMembersMapping.Accessor.Namespace, null);
			}
			ExportRootIfNecessary(xmlMembersMapping.Scope);
			return new XmlQualifiedName(xmlMembersMapping.Accessor.Name, xmlMembersMapping.Accessor.Namespace);
		}
		return null;
	}

	public void ExportMembersMapping(XmlMembersMapping xmlMembersMapping)
	{
		ExportMembersMapping(xmlMembersMapping, exportEnclosingType: true);
	}

	public void ExportMembersMapping(XmlMembersMapping xmlMembersMapping, bool exportEnclosingType)
	{
		xmlMembersMapping.CheckShallow();
		MembersMapping membersMapping = (MembersMapping)xmlMembersMapping.Accessor.Mapping;
		CheckScope(xmlMembersMapping.Scope);
		if (membersMapping.HasWrapperElement && exportEnclosingType)
		{
			ExportElement(xmlMembersMapping.Accessor);
		}
		else
		{
			MemberMapping[] members = membersMapping.Members;
			foreach (MemberMapping memberMapping in members)
			{
				if (memberMapping.Attribute != null)
				{
					throw new InvalidOperationException(Res.GetString("There was an error exporting '{0}': bare members cannot be attributes.", memberMapping.Attribute.Name));
				}
				if (memberMapping.Text != null)
				{
					throw new InvalidOperationException(Res.GetString("There was an error exporting '{0}': bare members cannot contain text content.", memberMapping.Text.Name));
				}
				if (memberMapping.Elements != null && memberMapping.Elements.Length != 0)
				{
					if (memberMapping.TypeDesc.IsArrayLike && !(memberMapping.Elements[0].Mapping is ArrayMapping))
					{
						throw new InvalidOperationException(Res.GetString("An element declared at the top level of a schema cannot have maxOccurs > 1. Provide a wrapper element for '{0}' by using XmlArray or XmlArrayItem instead of XmlElementAttribute, or by using the Wrapped parameter style.", memberMapping.Elements[0].Name));
					}
					if (exportEnclosingType)
					{
						ExportElement(memberMapping.Elements[0]);
					}
					else
					{
						ExportMapping(memberMapping.Elements[0].Mapping, memberMapping.Elements[0].Namespace, memberMapping.Elements[0].Any);
					}
				}
			}
		}
		ExportRootIfNecessary(xmlMembersMapping.Scope);
	}

	private static XmlSchemaType FindSchemaType(string name, XmlSchemaObjectCollection items)
	{
		foreach (XmlSchemaObject item in items)
		{
			if (item is XmlSchemaType xmlSchemaType && xmlSchemaType.Name == name)
			{
				return xmlSchemaType;
			}
		}
		return null;
	}

	private static bool IsAnyType(XmlSchemaType schemaType, bool mixed, bool unbounded)
	{
		if (schemaType is XmlSchemaComplexType xmlSchemaComplexType)
		{
			if (xmlSchemaComplexType.IsMixed != mixed)
			{
				return false;
			}
			if (xmlSchemaComplexType.Particle is XmlSchemaSequence)
			{
				XmlSchemaSequence xmlSchemaSequence = (XmlSchemaSequence)xmlSchemaComplexType.Particle;
				if (xmlSchemaSequence.Items.Count == 1 && xmlSchemaSequence.Items[0] is XmlSchemaAny)
				{
					XmlSchemaAny xmlSchemaAny = (XmlSchemaAny)xmlSchemaSequence.Items[0];
					return unbounded == xmlSchemaAny.IsMultipleOccurrence;
				}
			}
		}
		return false;
	}

	public string ExportAnyType(string ns)
	{
		string text = "any";
		int num = 0;
		XmlSchema xmlSchema = schemas[ns];
		if (xmlSchema != null)
		{
			while (true)
			{
				XmlSchemaType xmlSchemaType = FindSchemaType(text, xmlSchema.Items);
				if (xmlSchemaType == null)
				{
					break;
				}
				if (IsAnyType(xmlSchemaType, mixed: true, unbounded: true))
				{
					return text;
				}
				num++;
				text = "any" + num.ToString(CultureInfo.InvariantCulture);
			}
		}
		XmlSchemaComplexType xmlSchemaComplexType = new XmlSchemaComplexType();
		xmlSchemaComplexType.Name = text;
		xmlSchemaComplexType.IsMixed = true;
		XmlSchemaSequence xmlSchemaSequence = new XmlSchemaSequence();
		XmlSchemaAny xmlSchemaAny = new XmlSchemaAny();
		xmlSchemaAny.MinOccurs = 0m;
		xmlSchemaAny.MaxOccurs = decimal.MaxValue;
		xmlSchemaSequence.Items.Add(xmlSchemaAny);
		xmlSchemaComplexType.Particle = xmlSchemaSequence;
		AddSchemaItem(xmlSchemaComplexType, ns, null);
		return text;
	}

	public string ExportAnyType(XmlMembersMapping members)
	{
		if (members.Count == 1 && members[0].Any && members[0].ElementName.Length == 0)
		{
			XmlMemberMapping xmlMemberMapping = members[0];
			string @namespace = xmlMemberMapping.Namespace;
			bool flag = xmlMemberMapping.Mapping.TypeDesc.IsArrayLike;
			bool flag2 = ((flag && xmlMemberMapping.Mapping.TypeDesc.ArrayElementTypeDesc != null) ? xmlMemberMapping.Mapping.TypeDesc.ArrayElementTypeDesc.IsMixed : xmlMemberMapping.Mapping.TypeDesc.IsMixed);
			if (flag2 && xmlMemberMapping.Mapping.TypeDesc.IsMixed)
			{
				flag = true;
			}
			string text = (flag2 ? "any" : (flag ? "anyElements" : "anyElement"));
			string text2 = text;
			int num = 0;
			XmlSchema xmlSchema = schemas[@namespace];
			if (xmlSchema != null)
			{
				while (true)
				{
					XmlSchemaType xmlSchemaType = FindSchemaType(text2, xmlSchema.Items);
					if (xmlSchemaType == null)
					{
						break;
					}
					if (IsAnyType(xmlSchemaType, flag2, flag))
					{
						return text2;
					}
					num++;
					text2 = text + num.ToString(CultureInfo.InvariantCulture);
				}
			}
			XmlSchemaComplexType xmlSchemaComplexType = new XmlSchemaComplexType();
			xmlSchemaComplexType.Name = text2;
			xmlSchemaComplexType.IsMixed = flag2;
			XmlSchemaSequence xmlSchemaSequence = new XmlSchemaSequence();
			XmlSchemaAny xmlSchemaAny = new XmlSchemaAny();
			xmlSchemaAny.MinOccurs = 0m;
			if (flag)
			{
				xmlSchemaAny.MaxOccurs = decimal.MaxValue;
			}
			xmlSchemaSequence.Items.Add(xmlSchemaAny);
			xmlSchemaComplexType.Particle = xmlSchemaSequence;
			AddSchemaItem(xmlSchemaComplexType, @namespace, null);
			return text2;
		}
		return null;
	}

	private void CheckScope(TypeScope scope)
	{
		if (this.scope == null)
		{
			this.scope = scope;
		}
		else if (this.scope != scope)
		{
			throw new InvalidOperationException(Res.GetString("Exported mappings must come from the same importer."));
		}
	}

	private XmlSchemaElement ExportElement(ElementAccessor accessor)
	{
		if (!accessor.Mapping.IncludeInSchema && !accessor.Mapping.TypeDesc.IsRoot)
		{
			return null;
		}
		if (accessor.Any && accessor.Name.Length == 0)
		{
			throw new InvalidOperationException(Res.GetString("Cannot use wildcards at the top level of a schema."));
		}
		XmlSchemaElement xmlSchemaElement = (XmlSchemaElement)elements[accessor];
		if (xmlSchemaElement != null)
		{
			return xmlSchemaElement;
		}
		xmlSchemaElement = new XmlSchemaElement();
		xmlSchemaElement.Name = accessor.Name;
		xmlSchemaElement.IsNillable = accessor.IsNullable;
		elements.Add(accessor, xmlSchemaElement);
		xmlSchemaElement.Form = accessor.Form;
		AddSchemaItem(xmlSchemaElement, accessor.Namespace, null);
		ExportElementMapping(xmlSchemaElement, accessor.Mapping, accessor.Namespace, accessor.Any);
		return xmlSchemaElement;
	}

	private void CheckForDuplicateType(TypeMapping mapping, string newNamespace)
	{
		if (mapping.IsAnonymousType)
		{
			return;
		}
		string typeName = mapping.TypeName;
		XmlSchema xmlSchema = schemas[newNamespace];
		if (xmlSchema == null)
		{
			return;
		}
		foreach (XmlSchemaObject item in xmlSchema.Items)
		{
			if (item is XmlSchemaType xmlSchemaType && xmlSchemaType.Name == typeName)
			{
				throw new InvalidOperationException(Res.GetString("A type with the name {0} has already been added in namespace {1}.", typeName, newNamespace));
			}
		}
	}

	private XmlSchema AddSchema(string targetNamespace)
	{
		XmlSchema xmlSchema = new XmlSchema();
		xmlSchema.TargetNamespace = (string.IsNullOrEmpty(targetNamespace) ? null : targetNamespace);
		xmlSchema.ElementFormDefault = XmlSchemaForm.Qualified;
		xmlSchema.AttributeFormDefault = XmlSchemaForm.None;
		schemas.Add(xmlSchema);
		return xmlSchema;
	}

	private void AddSchemaItem(XmlSchemaObject item, string ns, string referencingNs)
	{
		XmlSchema xmlSchema = schemas[ns];
		if (xmlSchema == null)
		{
			xmlSchema = AddSchema(ns);
		}
		if (item is XmlSchemaElement)
		{
			XmlSchemaElement xmlSchemaElement = (XmlSchemaElement)item;
			if (xmlSchemaElement.Form == XmlSchemaForm.Unqualified)
			{
				throw new InvalidOperationException(Res.GetString("There was an error exporting '{0}': elements declared at the top level of a schema cannot be unqualified.", xmlSchemaElement.Name));
			}
			xmlSchemaElement.Form = XmlSchemaForm.None;
		}
		else if (item is XmlSchemaAttribute)
		{
			XmlSchemaAttribute xmlSchemaAttribute = (XmlSchemaAttribute)item;
			if (xmlSchemaAttribute.Form == XmlSchemaForm.Unqualified)
			{
				throw new InvalidOperationException(Res.GetString("There was an error exporting '{0}': elements declared at the top level of a schema cannot be unqualified.", xmlSchemaAttribute.Name));
			}
			xmlSchemaAttribute.Form = XmlSchemaForm.None;
		}
		xmlSchema.Items.Add(item);
		AddSchemaImport(ns, referencingNs);
	}

	private void AddSchemaImport(string ns, string referencingNs)
	{
		if (referencingNs == null || NamespacesEqual(ns, referencingNs))
		{
			return;
		}
		XmlSchema xmlSchema = schemas[referencingNs];
		if (xmlSchema == null)
		{
			xmlSchema = AddSchema(referencingNs);
		}
		if (FindImport(xmlSchema, ns) == null)
		{
			XmlSchemaImport xmlSchemaImport = new XmlSchemaImport();
			if (ns != null && ns.Length > 0)
			{
				xmlSchemaImport.Namespace = ns;
			}
			xmlSchema.Includes.Add(xmlSchemaImport);
		}
	}

	private static bool NamespacesEqual(string ns1, string ns2)
	{
		if (ns1 == null || ns1.Length == 0)
		{
			if (ns2 != null)
			{
				return ns2.Length == 0;
			}
			return true;
		}
		return ns1 == ns2;
	}

	private bool SchemaContainsItem(XmlSchemaObject item, string ns)
	{
		return schemas[ns]?.Items.Contains(item) ?? false;
	}

	private XmlSchemaImport FindImport(XmlSchema schema, string ns)
	{
		foreach (XmlSchemaObject include in schema.Includes)
		{
			if (include is XmlSchemaImport)
			{
				XmlSchemaImport xmlSchemaImport = (XmlSchemaImport)include;
				if (NamespacesEqual(xmlSchemaImport.Namespace, ns))
				{
					return xmlSchemaImport;
				}
			}
		}
		return null;
	}

	private void ExportMapping(Mapping mapping, string ns, bool isAny)
	{
		if (mapping is ArrayMapping)
		{
			ExportArrayMapping((ArrayMapping)mapping, ns, null);
			return;
		}
		if (mapping is PrimitiveMapping)
		{
			ExportPrimitiveMapping((PrimitiveMapping)mapping, ns);
			return;
		}
		if (mapping is StructMapping)
		{
			ExportStructMapping((StructMapping)mapping, ns, null);
			return;
		}
		if (mapping is MembersMapping)
		{
			ExportMembersMapping((MembersMapping)mapping, ns);
			return;
		}
		if (mapping is SpecialMapping)
		{
			ExportSpecialMapping((SpecialMapping)mapping, ns, isAny, null);
			return;
		}
		if (mapping is NullableMapping)
		{
			ExportMapping(((NullableMapping)mapping).BaseMapping, ns, isAny);
			return;
		}
		throw new ArgumentException(Res.GetString("Internal error."), "mapping");
	}

	private void ExportElementMapping(XmlSchemaElement element, Mapping mapping, string ns, bool isAny)
	{
		if (mapping is ArrayMapping)
		{
			ExportArrayMapping((ArrayMapping)mapping, ns, element);
		}
		else if (mapping is PrimitiveMapping)
		{
			PrimitiveMapping primitiveMapping = (PrimitiveMapping)mapping;
			if (primitiveMapping.IsAnonymousType)
			{
				element.SchemaType = ExportAnonymousPrimitiveMapping(primitiveMapping);
			}
			else
			{
				element.SchemaTypeName = ExportPrimitiveMapping(primitiveMapping, ns);
			}
		}
		else if (mapping is StructMapping)
		{
			ExportStructMapping((StructMapping)mapping, ns, element);
		}
		else if (mapping is MembersMapping)
		{
			element.SchemaType = ExportMembersMapping((MembersMapping)mapping, ns);
		}
		else if (mapping is SpecialMapping)
		{
			ExportSpecialMapping((SpecialMapping)mapping, ns, isAny, element);
		}
		else
		{
			if (!(mapping is NullableMapping))
			{
				throw new ArgumentException(Res.GetString("Internal error."), "mapping");
			}
			ExportElementMapping(element, ((NullableMapping)mapping).BaseMapping, ns, isAny);
		}
	}

	private XmlQualifiedName ExportNonXsdPrimitiveMapping(PrimitiveMapping mapping, string ns)
	{
		XmlSchemaSimpleType item = (XmlSchemaSimpleType)mapping.TypeDesc.DataType;
		if (!SchemaContainsItem(item, "http://microsoft.com/wsdl/types/"))
		{
			AddSchemaItem(item, "http://microsoft.com/wsdl/types/", ns);
		}
		else
		{
			AddSchemaImport(mapping.Namespace, ns);
		}
		return new XmlQualifiedName(mapping.TypeDesc.DataType.Name, "http://microsoft.com/wsdl/types/");
	}

	private XmlSchemaType ExportSpecialMapping(SpecialMapping mapping, string ns, bool isAny, XmlSchemaElement element)
	{
		switch (mapping.TypeDesc.Kind)
		{
		case TypeKind.Node:
		{
			XmlSchemaComplexType xmlSchemaComplexType4 = new XmlSchemaComplexType();
			xmlSchemaComplexType4.IsMixed = mapping.TypeDesc.IsMixed;
			XmlSchemaSequence xmlSchemaSequence4 = new XmlSchemaSequence();
			XmlSchemaAny xmlSchemaAny3 = new XmlSchemaAny();
			if (isAny)
			{
				xmlSchemaComplexType4.AnyAttribute = new XmlSchemaAnyAttribute();
				xmlSchemaComplexType4.IsMixed = true;
				xmlSchemaAny3.MaxOccurs = decimal.MaxValue;
			}
			xmlSchemaSequence4.Items.Add(xmlSchemaAny3);
			xmlSchemaComplexType4.Particle = xmlSchemaSequence4;
			if (element != null)
			{
				element.SchemaType = xmlSchemaComplexType4;
			}
			return xmlSchemaComplexType4;
		}
		case TypeKind.Serializable:
		{
			SerializableMapping serializableMapping = (SerializableMapping)mapping;
			if (serializableMapping.IsAny)
			{
				XmlSchemaComplexType xmlSchemaComplexType = new XmlSchemaComplexType();
				xmlSchemaComplexType.IsMixed = mapping.TypeDesc.IsMixed;
				XmlSchemaSequence xmlSchemaSequence = new XmlSchemaSequence();
				XmlSchemaAny xmlSchemaAny = new XmlSchemaAny();
				if (isAny)
				{
					xmlSchemaComplexType.AnyAttribute = new XmlSchemaAnyAttribute();
					xmlSchemaComplexType.IsMixed = true;
					xmlSchemaAny.MaxOccurs = decimal.MaxValue;
				}
				if (serializableMapping.NamespaceList.Length > 0)
				{
					xmlSchemaAny.Namespace = serializableMapping.NamespaceList;
				}
				xmlSchemaAny.ProcessContents = XmlSchemaContentProcessing.Lax;
				if (serializableMapping.Schemas != null)
				{
					foreach (XmlSchema item in serializableMapping.Schemas.Schemas())
					{
						if (item.TargetNamespace != "http://www.w3.org/2001/XMLSchema")
						{
							schemas.Add(item, delay: true);
							AddSchemaImport(item.TargetNamespace, ns);
						}
					}
				}
				xmlSchemaSequence.Items.Add(xmlSchemaAny);
				xmlSchemaComplexType.Particle = xmlSchemaSequence;
				if (element != null)
				{
					element.SchemaType = xmlSchemaComplexType;
				}
				return xmlSchemaComplexType;
			}
			if (serializableMapping.XsiType != null || serializableMapping.XsdType != null)
			{
				XmlSchemaType xmlSchemaType = serializableMapping.XsdType;
				foreach (XmlSchema item2 in serializableMapping.Schemas.Schemas())
				{
					if (item2.TargetNamespace != "http://www.w3.org/2001/XMLSchema")
					{
						schemas.Add(item2, delay: true);
						AddSchemaImport(item2.TargetNamespace, ns);
						if (!serializableMapping.XsiType.IsEmpty && serializableMapping.XsiType.Namespace == item2.TargetNamespace)
						{
							xmlSchemaType = (XmlSchemaType)item2.SchemaTypes[serializableMapping.XsiType];
						}
					}
				}
				if (element != null)
				{
					element.SchemaTypeName = serializableMapping.XsiType;
					if (element.SchemaTypeName.IsEmpty)
					{
						element.SchemaType = xmlSchemaType;
					}
				}
				serializableMapping.CheckDuplicateElement(element, ns);
				return xmlSchemaType;
			}
			if (serializableMapping.Schema != null)
			{
				XmlSchemaComplexType xmlSchemaComplexType2 = new XmlSchemaComplexType();
				XmlSchemaAny xmlSchemaAny2 = new XmlSchemaAny();
				XmlSchemaSequence xmlSchemaSequence2 = new XmlSchemaSequence();
				xmlSchemaSequence2.Items.Add(xmlSchemaAny2);
				xmlSchemaComplexType2.Particle = xmlSchemaSequence2;
				string targetNamespace = serializableMapping.Schema.TargetNamespace;
				xmlSchemaAny2.Namespace = ((targetNamespace == null) ? "" : targetNamespace);
				XmlSchema xmlSchema3 = schemas[targetNamespace];
				if (xmlSchema3 == null)
				{
					schemas.Add(serializableMapping.Schema);
				}
				else if (xmlSchema3 != serializableMapping.Schema)
				{
					throw new InvalidOperationException(Res.GetString("The namespace, {0}, is a duplicate.", targetNamespace));
				}
				if (element != null)
				{
					element.SchemaType = xmlSchemaComplexType2;
				}
				serializableMapping.CheckDuplicateElement(element, ns);
				return xmlSchemaComplexType2;
			}
			XmlSchemaComplexType xmlSchemaComplexType3 = new XmlSchemaComplexType();
			XmlSchemaElement xmlSchemaElement = new XmlSchemaElement();
			xmlSchemaElement.RefName = new XmlQualifiedName("schema", "http://www.w3.org/2001/XMLSchema");
			XmlSchemaSequence xmlSchemaSequence3 = new XmlSchemaSequence();
			xmlSchemaSequence3.Items.Add(xmlSchemaElement);
			xmlSchemaSequence3.Items.Add(new XmlSchemaAny());
			xmlSchemaComplexType3.Particle = xmlSchemaSequence3;
			AddSchemaImport("http://www.w3.org/2001/XMLSchema", ns);
			if (element != null)
			{
				element.SchemaType = xmlSchemaComplexType3;
			}
			return xmlSchemaComplexType3;
		}
		default:
			throw new ArgumentException(Res.GetString("Internal error."), "mapping");
		}
	}

	private XmlSchemaType ExportMembersMapping(MembersMapping mapping, string ns)
	{
		XmlSchemaComplexType xmlSchemaComplexType = new XmlSchemaComplexType();
		ExportTypeMembers(xmlSchemaComplexType, mapping.Members, mapping.TypeName, ns, hasSimpleContent: false, openModel: false);
		if (mapping.XmlnsMember != null)
		{
			AddXmlnsAnnotation(xmlSchemaComplexType, mapping.XmlnsMember.Name);
		}
		return xmlSchemaComplexType;
	}

	private XmlSchemaType ExportAnonymousPrimitiveMapping(PrimitiveMapping mapping)
	{
		if (mapping is EnumMapping)
		{
			return ExportEnumMapping((EnumMapping)mapping, null);
		}
		throw new InvalidOperationException(Res.GetString("Internal error: {0}.", "Unsuported anonymous mapping type: " + mapping.ToString()));
	}

	private XmlQualifiedName ExportPrimitiveMapping(PrimitiveMapping mapping, string ns)
	{
		if (mapping is EnumMapping)
		{
			return new XmlQualifiedName(ExportEnumMapping((EnumMapping)mapping, ns).Name, mapping.Namespace);
		}
		if (mapping.TypeDesc.IsXsdType)
		{
			return new XmlQualifiedName(mapping.TypeDesc.DataType.Name, "http://www.w3.org/2001/XMLSchema");
		}
		return ExportNonXsdPrimitiveMapping(mapping, ns);
	}

	private void ExportArrayMapping(ArrayMapping mapping, string ns, XmlSchemaElement element)
	{
		ArrayMapping arrayMapping = mapping;
		while (arrayMapping.Next != null)
		{
			arrayMapping = arrayMapping.Next;
		}
		XmlSchemaComplexType xmlSchemaComplexType = (XmlSchemaComplexType)types[arrayMapping];
		if (xmlSchemaComplexType == null)
		{
			CheckForDuplicateType(arrayMapping, arrayMapping.Namespace);
			xmlSchemaComplexType = new XmlSchemaComplexType();
			if (!mapping.IsAnonymousType)
			{
				xmlSchemaComplexType.Name = mapping.TypeName;
				AddSchemaItem(xmlSchemaComplexType, mapping.Namespace, ns);
			}
			if (!arrayMapping.IsAnonymousType)
			{
				types.Add(arrayMapping, xmlSchemaComplexType);
			}
			XmlSchemaSequence xmlSchemaSequence = new XmlSchemaSequence();
			ExportElementAccessors(xmlSchemaSequence, mapping.Elements, repeats: true, valueTypeOptional: false, mapping.Namespace);
			if (xmlSchemaSequence.Items.Count > 0)
			{
				if (xmlSchemaSequence.Items[0] is XmlSchemaChoice)
				{
					xmlSchemaComplexType.Particle = (XmlSchemaChoice)xmlSchemaSequence.Items[0];
				}
				else
				{
					xmlSchemaComplexType.Particle = xmlSchemaSequence;
				}
			}
		}
		else
		{
			AddSchemaImport(mapping.Namespace, ns);
		}
		if (element != null)
		{
			if (mapping.IsAnonymousType)
			{
				element.SchemaType = xmlSchemaComplexType;
			}
			else
			{
				element.SchemaTypeName = new XmlQualifiedName(xmlSchemaComplexType.Name, mapping.Namespace);
			}
		}
	}

	private void ExportElementAccessors(XmlSchemaGroupBase group, ElementAccessor[] accessors, bool repeats, bool valueTypeOptional, string ns)
	{
		if (accessors.Length == 0)
		{
			return;
		}
		if (accessors.Length == 1)
		{
			ExportElementAccessor(group, accessors[0], repeats, valueTypeOptional, ns);
			return;
		}
		XmlSchemaChoice xmlSchemaChoice = new XmlSchemaChoice();
		xmlSchemaChoice.MaxOccurs = (repeats ? decimal.MaxValue : 1m);
		xmlSchemaChoice.MinOccurs = ((!repeats) ? 1 : 0);
		for (int i = 0; i < accessors.Length; i++)
		{
			ExportElementAccessor(xmlSchemaChoice, accessors[i], repeats: false, valueTypeOptional, ns);
		}
		if (xmlSchemaChoice.Items.Count > 0)
		{
			group.Items.Add(xmlSchemaChoice);
		}
	}

	private void ExportAttributeAccessor(XmlSchemaComplexType type, AttributeAccessor accessor, bool valueTypeOptional, string ns)
	{
		if (accessor == null)
		{
			return;
		}
		XmlSchemaObjectCollection xmlSchemaObjectCollection;
		if (type.ContentModel != null)
		{
			if (type.ContentModel.Content is XmlSchemaComplexContentRestriction)
			{
				xmlSchemaObjectCollection = ((XmlSchemaComplexContentRestriction)type.ContentModel.Content).Attributes;
			}
			else if (type.ContentModel.Content is XmlSchemaComplexContentExtension)
			{
				xmlSchemaObjectCollection = ((XmlSchemaComplexContentExtension)type.ContentModel.Content).Attributes;
			}
			else
			{
				if (!(type.ContentModel.Content is XmlSchemaSimpleContentExtension))
				{
					throw new InvalidOperationException(Res.GetString("Invalid content {0}.", type.ContentModel.Content.GetType().Name));
				}
				xmlSchemaObjectCollection = ((XmlSchemaSimpleContentExtension)type.ContentModel.Content).Attributes;
			}
		}
		else
		{
			xmlSchemaObjectCollection = type.Attributes;
		}
		if (accessor.IsSpecialXmlNamespace)
		{
			AddSchemaImport("http://www.w3.org/XML/1998/namespace", ns);
			XmlSchemaAttribute xmlSchemaAttribute = new XmlSchemaAttribute();
			xmlSchemaAttribute.Use = XmlSchemaUse.Optional;
			xmlSchemaAttribute.RefName = new XmlQualifiedName(accessor.Name, "http://www.w3.org/XML/1998/namespace");
			xmlSchemaObjectCollection.Add(xmlSchemaAttribute);
			return;
		}
		if (accessor.Any)
		{
			if (type.ContentModel == null)
			{
				type.AnyAttribute = new XmlSchemaAnyAttribute();
				return;
			}
			XmlSchemaContent content = type.ContentModel.Content;
			if (content is XmlSchemaComplexContentExtension)
			{
				((XmlSchemaComplexContentExtension)content).AnyAttribute = new XmlSchemaAnyAttribute();
			}
			else if (content is XmlSchemaComplexContentRestriction)
			{
				((XmlSchemaComplexContentRestriction)content).AnyAttribute = new XmlSchemaAnyAttribute();
			}
			else if (type.ContentModel.Content is XmlSchemaSimpleContentExtension)
			{
				((XmlSchemaSimpleContentExtension)content).AnyAttribute = new XmlSchemaAnyAttribute();
			}
			return;
		}
		XmlSchemaAttribute xmlSchemaAttribute2 = new XmlSchemaAttribute();
		xmlSchemaAttribute2.Use = XmlSchemaUse.None;
		if (!accessor.HasDefault && !valueTypeOptional && accessor.Mapping.TypeDesc.IsValueType)
		{
			xmlSchemaAttribute2.Use = XmlSchemaUse.Required;
		}
		xmlSchemaAttribute2.Name = accessor.Name;
		if (accessor.Namespace == null || accessor.Namespace == ns)
		{
			XmlSchema xmlSchema = schemas[ns];
			if (xmlSchema == null)
			{
				xmlSchemaAttribute2.Form = ((accessor.Form != XmlSchemaForm.Unqualified) ? accessor.Form : XmlSchemaForm.None);
			}
			else
			{
				xmlSchemaAttribute2.Form = ((accessor.Form != xmlSchema.AttributeFormDefault) ? accessor.Form : XmlSchemaForm.None);
			}
			xmlSchemaObjectCollection.Add(xmlSchemaAttribute2);
		}
		else
		{
			if (attributes[accessor] == null)
			{
				xmlSchemaAttribute2.Use = XmlSchemaUse.None;
				xmlSchemaAttribute2.Form = accessor.Form;
				AddSchemaItem(xmlSchemaAttribute2, accessor.Namespace, ns);
				attributes.Add(accessor, accessor);
			}
			XmlSchemaAttribute xmlSchemaAttribute3 = new XmlSchemaAttribute();
			xmlSchemaAttribute3.Use = XmlSchemaUse.None;
			xmlSchemaAttribute3.RefName = new XmlQualifiedName(accessor.Name, accessor.Namespace);
			xmlSchemaObjectCollection.Add(xmlSchemaAttribute3);
			AddSchemaImport(accessor.Namespace, ns);
		}
		if (accessor.Mapping is PrimitiveMapping)
		{
			PrimitiveMapping primitiveMapping = (PrimitiveMapping)accessor.Mapping;
			if (primitiveMapping.IsList)
			{
				XmlSchemaSimpleType xmlSchemaSimpleType = new XmlSchemaSimpleType();
				XmlSchemaSimpleTypeList xmlSchemaSimpleTypeList = new XmlSchemaSimpleTypeList();
				if (primitiveMapping.IsAnonymousType)
				{
					xmlSchemaSimpleTypeList.ItemType = (XmlSchemaSimpleType)ExportAnonymousPrimitiveMapping(primitiveMapping);
				}
				else
				{
					xmlSchemaSimpleTypeList.ItemTypeName = ExportPrimitiveMapping(primitiveMapping, (accessor.Namespace == null) ? ns : accessor.Namespace);
				}
				xmlSchemaSimpleType.Content = xmlSchemaSimpleTypeList;
				xmlSchemaAttribute2.SchemaType = xmlSchemaSimpleType;
			}
			else if (primitiveMapping.IsAnonymousType)
			{
				xmlSchemaAttribute2.SchemaType = (XmlSchemaSimpleType)ExportAnonymousPrimitiveMapping(primitiveMapping);
			}
			else
			{
				xmlSchemaAttribute2.SchemaTypeName = ExportPrimitiveMapping(primitiveMapping, (accessor.Namespace == null) ? ns : accessor.Namespace);
			}
		}
		else if (!(accessor.Mapping is SpecialMapping))
		{
			throw new InvalidOperationException(Res.GetString("Internal error."));
		}
		if (accessor.HasDefault)
		{
			xmlSchemaAttribute2.DefaultValue = ExportDefaultValue(accessor.Mapping, accessor.Default);
		}
	}

	private void ExportElementAccessor(XmlSchemaGroupBase group, ElementAccessor accessor, bool repeats, bool valueTypeOptional, string ns)
	{
		if (accessor.Any && accessor.Name.Length == 0)
		{
			XmlSchemaAny xmlSchemaAny = new XmlSchemaAny();
			xmlSchemaAny.MinOccurs = 0m;
			xmlSchemaAny.MaxOccurs = (repeats ? decimal.MaxValue : 1m);
			if (accessor.Namespace != null && accessor.Namespace.Length > 0 && accessor.Namespace != ns)
			{
				xmlSchemaAny.Namespace = accessor.Namespace;
			}
			group.Items.Add(xmlSchemaAny);
			return;
		}
		XmlSchemaElement xmlSchemaElement = (XmlSchemaElement)elements[accessor];
		int num = ((!(repeats || accessor.HasDefault || (!accessor.IsNullable && !accessor.Mapping.TypeDesc.IsValueType) || valueTypeOptional)) ? 1 : 0);
		decimal maxOccurs = ((repeats || accessor.IsUnbounded) ? decimal.MaxValue : 1m);
		if (xmlSchemaElement == null)
		{
			xmlSchemaElement = new XmlSchemaElement();
			xmlSchemaElement.IsNillable = accessor.IsNullable;
			xmlSchemaElement.Name = accessor.Name;
			if (accessor.HasDefault)
			{
				xmlSchemaElement.DefaultValue = ExportDefaultValue(accessor.Mapping, accessor.Default);
			}
			if (accessor.IsTopLevelInSchema)
			{
				elements.Add(accessor, xmlSchemaElement);
				xmlSchemaElement.Form = accessor.Form;
				AddSchemaItem(xmlSchemaElement, accessor.Namespace, ns);
			}
			else
			{
				xmlSchemaElement.MinOccurs = num;
				xmlSchemaElement.MaxOccurs = maxOccurs;
				XmlSchema xmlSchema = schemas[ns];
				if (xmlSchema == null)
				{
					xmlSchemaElement.Form = ((accessor.Form != XmlSchemaForm.Qualified) ? accessor.Form : XmlSchemaForm.None);
				}
				else
				{
					xmlSchemaElement.Form = ((accessor.Form != xmlSchema.ElementFormDefault) ? accessor.Form : XmlSchemaForm.None);
				}
			}
			ExportElementMapping(xmlSchemaElement, accessor.Mapping, accessor.Namespace, accessor.Any);
		}
		if (accessor.IsTopLevelInSchema)
		{
			XmlSchemaElement xmlSchemaElement2 = new XmlSchemaElement();
			xmlSchemaElement2.RefName = new XmlQualifiedName(accessor.Name, accessor.Namespace);
			xmlSchemaElement2.MinOccurs = num;
			xmlSchemaElement2.MaxOccurs = maxOccurs;
			group.Items.Add(xmlSchemaElement2);
			AddSchemaImport(accessor.Namespace, ns);
		}
		else
		{
			group.Items.Add(xmlSchemaElement);
		}
	}

	internal static string ExportDefaultValue(TypeMapping mapping, object value)
	{
		if (!(mapping is PrimitiveMapping))
		{
			return null;
		}
		if (value == null || value == DBNull.Value)
		{
			return null;
		}
		if (mapping is EnumMapping)
		{
			EnumMapping enumMapping = (EnumMapping)mapping;
			ConstantMapping[] constants = enumMapping.Constants;
			if (enumMapping.IsFlags)
			{
				string[] array = new string[constants.Length];
				long[] array2 = new long[constants.Length];
				Hashtable hashtable = new Hashtable();
				for (int i = 0; i < constants.Length; i++)
				{
					array[i] = constants[i].XmlName;
					array2[i] = 1 << i;
					hashtable.Add(constants[i].Name, array2[i]);
				}
				long num = XmlCustomFormatter.ToEnum((string)value, hashtable, enumMapping.TypeName, validate: false);
				if (num == 0L)
				{
					return null;
				}
				return XmlCustomFormatter.FromEnum(num, array, array2, mapping.TypeDesc.FullName);
			}
			for (int j = 0; j < constants.Length; j++)
			{
				if (constants[j].Name == (string)value)
				{
					return constants[j].XmlName;
				}
			}
			return null;
		}
		PrimitiveMapping primitiveMapping = (PrimitiveMapping)mapping;
		if (!primitiveMapping.TypeDesc.HasCustomFormatter)
		{
			if (primitiveMapping.TypeDesc.FormatterName == "String")
			{
				return (string)value;
			}
			Type typeFromHandle = typeof(XmlConvert);
			MethodInfo method = typeFromHandle.GetMethod("ToString", new Type[1] { primitiveMapping.TypeDesc.Type });
			if (method != null)
			{
				return (string)method.Invoke(typeFromHandle, new object[1] { value });
			}
			throw new InvalidOperationException(Res.GetString("Value '{0}' cannot be converted to {1}.", value.ToString(), primitiveMapping.TypeDesc.Name));
		}
		string text = XmlCustomFormatter.FromDefaultValue(value, primitiveMapping.TypeDesc.FormatterName);
		if (text == null)
		{
			throw new InvalidOperationException(Res.GetString("Value '{0}' cannot be converted to {1}.", value.ToString(), primitiveMapping.TypeDesc.Name));
		}
		return text;
	}

	private void ExportRootIfNecessary(TypeScope typeScope)
	{
		if (!needToExportRoot)
		{
			return;
		}
		foreach (TypeMapping typeMapping in typeScope.TypeMappings)
		{
			if (typeMapping is StructMapping && typeMapping.TypeDesc.IsRoot)
			{
				ExportDerivedMappings((StructMapping)typeMapping);
			}
			else if (typeMapping is ArrayMapping)
			{
				ExportArrayMapping((ArrayMapping)typeMapping, typeMapping.Namespace, null);
			}
			else if (typeMapping is SerializableMapping)
			{
				ExportSpecialMapping((SerializableMapping)typeMapping, typeMapping.Namespace, isAny: false, null);
			}
		}
	}

	private XmlQualifiedName ExportStructMapping(StructMapping mapping, string ns, XmlSchemaElement element)
	{
		if (mapping.TypeDesc.IsRoot)
		{
			needToExportRoot = true;
			return XmlQualifiedName.Empty;
		}
		if (mapping.IsAnonymousType)
		{
			if (references[mapping] != null)
			{
				throw new InvalidOperationException(Res.GetString("A circular type reference was detected in anonymous type '{0}'.  Please change '{0}' to be a named type by setting {1}={2} in the type definition.", mapping.TypeDesc.Name, "AnonymousType", "false"));
			}
			references[mapping] = mapping;
		}
		XmlSchemaComplexType xmlSchemaComplexType = (XmlSchemaComplexType)types[mapping];
		if (xmlSchemaComplexType == null)
		{
			if (!mapping.IncludeInSchema)
			{
				throw new InvalidOperationException(Res.GetString("The type {0} may not be exported to a schema because the IncludeInSchema property of the XmlType attribute is 'false'.", mapping.TypeDesc.Name));
			}
			CheckForDuplicateType(mapping, mapping.Namespace);
			xmlSchemaComplexType = new XmlSchemaComplexType();
			if (!mapping.IsAnonymousType)
			{
				xmlSchemaComplexType.Name = mapping.TypeName;
				AddSchemaItem(xmlSchemaComplexType, mapping.Namespace, ns);
				types.Add(mapping, xmlSchemaComplexType);
			}
			xmlSchemaComplexType.IsAbstract = mapping.TypeDesc.IsAbstract;
			bool openModel = mapping.IsOpenModel;
			if (mapping.BaseMapping != null && mapping.BaseMapping.IncludeInSchema)
			{
				if (mapping.BaseMapping.IsAnonymousType)
				{
					throw new InvalidOperationException(Res.GetString("Illegal type derivation: Type '{0}' derives from anonymous type '{1}'. Please change '{1}' to be a named type by setting {2}={3} in the type definition.", mapping.TypeDesc.Name, mapping.BaseMapping.TypeDesc.Name, "AnonymousType", "false"));
				}
				if (mapping.HasSimpleContent)
				{
					XmlSchemaSimpleContent xmlSchemaSimpleContent = new XmlSchemaSimpleContent();
					XmlSchemaSimpleContentExtension xmlSchemaSimpleContentExtension = new XmlSchemaSimpleContentExtension();
					xmlSchemaSimpleContentExtension.BaseTypeName = ExportStructMapping(mapping.BaseMapping, mapping.Namespace, null);
					xmlSchemaSimpleContent.Content = xmlSchemaSimpleContentExtension;
					xmlSchemaComplexType.ContentModel = xmlSchemaSimpleContent;
				}
				else
				{
					XmlSchemaComplexContentExtension xmlSchemaComplexContentExtension = new XmlSchemaComplexContentExtension();
					xmlSchemaComplexContentExtension.BaseTypeName = ExportStructMapping(mapping.BaseMapping, mapping.Namespace, null);
					XmlSchemaComplexContent xmlSchemaComplexContent = new XmlSchemaComplexContent();
					xmlSchemaComplexContent.Content = xmlSchemaComplexContentExtension;
					xmlSchemaComplexContent.IsMixed = XmlSchemaImporter.IsMixed((XmlSchemaComplexType)types[mapping.BaseMapping]);
					xmlSchemaComplexType.ContentModel = xmlSchemaComplexContent;
				}
				openModel = false;
			}
			ExportTypeMembers(xmlSchemaComplexType, mapping.Members, mapping.TypeName, mapping.Namespace, mapping.HasSimpleContent, openModel);
			ExportDerivedMappings(mapping);
			if (mapping.XmlnsMember != null)
			{
				AddXmlnsAnnotation(xmlSchemaComplexType, mapping.XmlnsMember.Name);
			}
		}
		else
		{
			AddSchemaImport(mapping.Namespace, ns);
		}
		if (mapping.IsAnonymousType)
		{
			references[mapping] = null;
			if (element != null)
			{
				element.SchemaType = xmlSchemaComplexType;
			}
			return XmlQualifiedName.Empty;
		}
		XmlQualifiedName xmlQualifiedName = new XmlQualifiedName(xmlSchemaComplexType.Name, mapping.Namespace);
		if (element != null)
		{
			element.SchemaTypeName = xmlQualifiedName;
		}
		return xmlQualifiedName;
	}

	private void ExportTypeMembers(XmlSchemaComplexType type, MemberMapping[] members, string name, string ns, bool hasSimpleContent, bool openModel)
	{
		XmlSchemaGroupBase xmlSchemaGroupBase = new XmlSchemaSequence();
		TypeMapping typeMapping = null;
		foreach (MemberMapping memberMapping in members)
		{
			if (memberMapping.Ignore)
			{
				continue;
			}
			if (memberMapping.Text != null)
			{
				if (typeMapping != null)
				{
					throw new InvalidOperationException(Res.GetString("Cannot serialize object of type '{0}' because it has multiple XmlText attributes. Consider using an array of strings with XmlTextAttribute for serialization of a mixed complex type.", name));
				}
				typeMapping = memberMapping.Text.Mapping;
			}
			if (memberMapping.Elements.Length != 0)
			{
				bool repeats = memberMapping.TypeDesc.IsArrayLike && (memberMapping.Elements.Length != 1 || !(memberMapping.Elements[0].Mapping is ArrayMapping));
				bool valueTypeOptional = memberMapping.CheckSpecified != 0 || memberMapping.CheckShouldPersist;
				ExportElementAccessors(xmlSchemaGroupBase, memberMapping.Elements, repeats, valueTypeOptional, ns);
			}
		}
		if (xmlSchemaGroupBase.Items.Count > 0)
		{
			if (type.ContentModel != null)
			{
				if (type.ContentModel.Content is XmlSchemaComplexContentRestriction)
				{
					((XmlSchemaComplexContentRestriction)type.ContentModel.Content).Particle = xmlSchemaGroupBase;
				}
				else
				{
					if (!(type.ContentModel.Content is XmlSchemaComplexContentExtension))
					{
						throw new InvalidOperationException(Res.GetString("Invalid content {0}.", type.ContentModel.Content.GetType().Name));
					}
					((XmlSchemaComplexContentExtension)type.ContentModel.Content).Particle = xmlSchemaGroupBase;
				}
			}
			else
			{
				type.Particle = xmlSchemaGroupBase;
			}
		}
		if (typeMapping != null)
		{
			if (hasSimpleContent)
			{
				if (typeMapping is PrimitiveMapping && xmlSchemaGroupBase.Items.Count == 0)
				{
					PrimitiveMapping primitiveMapping = (PrimitiveMapping)typeMapping;
					if (primitiveMapping.IsList)
					{
						type.IsMixed = true;
					}
					else
					{
						if (primitiveMapping.IsAnonymousType)
						{
							throw new InvalidOperationException(Res.GetString("Illegal type derivation: Type '{0}' derives from anonymous type '{1}'. Please change '{1}' to be a named type by setting {2}={3} in the type definition.", typeMapping.TypeDesc.Name, primitiveMapping.TypeDesc.Name, "AnonymousType", "false"));
						}
						XmlSchemaSimpleContent xmlSchemaSimpleContent = new XmlSchemaSimpleContent();
						XmlSchemaSimpleContentExtension xmlSchemaSimpleContentExtension = (XmlSchemaSimpleContentExtension)(xmlSchemaSimpleContent.Content = new XmlSchemaSimpleContentExtension());
						type.ContentModel = xmlSchemaSimpleContent;
						xmlSchemaSimpleContentExtension.BaseTypeName = ExportPrimitiveMapping(primitiveMapping, ns);
					}
				}
			}
			else
			{
				type.IsMixed = true;
			}
		}
		bool flag = false;
		for (int j = 0; j < members.Length; j++)
		{
			if (members[j].Attribute != null)
			{
				ExportAttributeAccessor(type, members[j].Attribute, members[j].CheckSpecified != 0 || members[j].CheckShouldPersist, ns);
				if (members[j].Attribute.Any)
				{
					flag = true;
				}
			}
		}
		if (openModel && !flag)
		{
			AttributeAccessor attributeAccessor = new AttributeAccessor();
			attributeAccessor.Any = true;
			ExportAttributeAccessor(type, attributeAccessor, valueTypeOptional: false, ns);
		}
	}

	private void ExportDerivedMappings(StructMapping mapping)
	{
		if (mapping.IsAnonymousType)
		{
			return;
		}
		for (StructMapping structMapping = mapping.DerivedMappings; structMapping != null; structMapping = structMapping.NextDerivedMapping)
		{
			if (structMapping.IncludeInSchema)
			{
				ExportStructMapping(structMapping, structMapping.Namespace, null);
			}
		}
	}

	private XmlSchemaType ExportEnumMapping(EnumMapping mapping, string ns)
	{
		if (!mapping.IncludeInSchema)
		{
			throw new InvalidOperationException(Res.GetString("The type {0} may not be exported to a schema because the IncludeInSchema property of the XmlType attribute is 'false'.", mapping.TypeDesc.Name));
		}
		XmlSchemaSimpleType xmlSchemaSimpleType = (XmlSchemaSimpleType)types[mapping];
		if (xmlSchemaSimpleType == null)
		{
			CheckForDuplicateType(mapping, mapping.Namespace);
			xmlSchemaSimpleType = new XmlSchemaSimpleType();
			xmlSchemaSimpleType.Name = mapping.TypeName;
			if (!mapping.IsAnonymousType)
			{
				types.Add(mapping, xmlSchemaSimpleType);
				AddSchemaItem(xmlSchemaSimpleType, mapping.Namespace, ns);
			}
			XmlSchemaSimpleTypeRestriction xmlSchemaSimpleTypeRestriction = new XmlSchemaSimpleTypeRestriction();
			xmlSchemaSimpleTypeRestriction.BaseTypeName = new XmlQualifiedName("string", "http://www.w3.org/2001/XMLSchema");
			for (int i = 0; i < mapping.Constants.Length; i++)
			{
				ConstantMapping constantMapping = mapping.Constants[i];
				XmlSchemaEnumerationFacet xmlSchemaEnumerationFacet = new XmlSchemaEnumerationFacet();
				xmlSchemaEnumerationFacet.Value = constantMapping.XmlName;
				xmlSchemaSimpleTypeRestriction.Facets.Add(xmlSchemaEnumerationFacet);
			}
			if (!mapping.IsFlags)
			{
				xmlSchemaSimpleType.Content = xmlSchemaSimpleTypeRestriction;
			}
			else
			{
				XmlSchemaSimpleTypeList xmlSchemaSimpleTypeList = new XmlSchemaSimpleTypeList();
				XmlSchemaSimpleType xmlSchemaSimpleType2 = new XmlSchemaSimpleType();
				xmlSchemaSimpleType2.Content = xmlSchemaSimpleTypeRestriction;
				xmlSchemaSimpleTypeList.ItemType = xmlSchemaSimpleType2;
				xmlSchemaSimpleType.Content = xmlSchemaSimpleTypeList;
			}
		}
		if (!mapping.IsAnonymousType)
		{
			AddSchemaImport(mapping.Namespace, ns);
		}
		return xmlSchemaSimpleType;
	}

	private void AddXmlnsAnnotation(XmlSchemaComplexType type, string xmlnsMemberName)
	{
		XmlSchemaAnnotation xmlSchemaAnnotation = new XmlSchemaAnnotation();
		XmlSchemaAppInfo xmlSchemaAppInfo = new XmlSchemaAppInfo();
		XmlDocument xmlDocument = new XmlDocument();
		XmlElement xmlElement = xmlDocument.CreateElement("keepNamespaceDeclarations");
		if (xmlnsMemberName != null)
		{
			xmlElement.InsertBefore(xmlDocument.CreateTextNode(xmlnsMemberName), null);
		}
		xmlSchemaAppInfo.Markup = new XmlNode[1] { xmlElement };
		xmlSchemaAnnotation.Items.Add(xmlSchemaAppInfo);
		type.Annotation = xmlSchemaAnnotation;
	}
}

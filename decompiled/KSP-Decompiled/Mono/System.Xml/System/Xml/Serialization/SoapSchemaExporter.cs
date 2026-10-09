using System.Collections;
using System.Xml.Schema;

namespace System.Xml.Serialization;

public class SoapSchemaExporter
{
	internal const XmlSchemaForm elementFormDefault = XmlSchemaForm.Qualified;

	private XmlSchemas schemas;

	private Hashtable types = new Hashtable();

	private bool exportedRoot;

	private TypeScope scope;

	private XmlDocument document;

	private static XmlQualifiedName ArrayQName = new XmlQualifiedName("Array", "http://schemas.xmlsoap.org/soap/encoding/");

	private static XmlQualifiedName ArrayTypeQName = new XmlQualifiedName("arrayType", "http://schemas.xmlsoap.org/soap/encoding/");

	internal XmlDocument Document
	{
		get
		{
			if (document == null)
			{
				document = new XmlDocument();
			}
			return document;
		}
	}

	public SoapSchemaExporter(XmlSchemas schemas)
	{
		this.schemas = schemas;
	}

	public void ExportTypeMapping(XmlTypeMapping xmlTypeMapping)
	{
		CheckScope(xmlTypeMapping.Scope);
		ExportTypeMapping(xmlTypeMapping.Mapping, null);
	}

	public void ExportMembersMapping(XmlMembersMapping xmlMembersMapping)
	{
		ExportMembersMapping(xmlMembersMapping, exportEnclosingType: false);
	}

	public void ExportMembersMapping(XmlMembersMapping xmlMembersMapping, bool exportEnclosingType)
	{
		CheckScope(xmlMembersMapping.Scope);
		MembersMapping membersMapping = (MembersMapping)xmlMembersMapping.Accessor.Mapping;
		if (exportEnclosingType)
		{
			ExportTypeMapping(membersMapping, null);
			return;
		}
		MemberMapping[] members = membersMapping.Members;
		foreach (MemberMapping memberMapping in members)
		{
			if (memberMapping.Elements.Length != 0)
			{
				ExportTypeMapping(memberMapping.Elements[0].Mapping, null);
			}
		}
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

	private void CheckForDuplicateType(string newTypeName, string newNamespace)
	{
		XmlSchema xmlSchema = schemas[newNamespace];
		if (xmlSchema == null)
		{
			return;
		}
		foreach (XmlSchemaObject item in xmlSchema.Items)
		{
			if (item is XmlSchemaType xmlSchemaType && xmlSchemaType.Name == newTypeName)
			{
				throw new InvalidOperationException(Res.GetString("A type with the name {0} has already been added in namespace {1}.", newTypeName, newNamespace));
			}
		}
	}

	private void AddSchemaItem(XmlSchemaObject item, string ns, string referencingNs)
	{
		if (!SchemaContainsItem(item, ns))
		{
			XmlSchema xmlSchema = schemas[ns];
			if (xmlSchema == null)
			{
				xmlSchema = new XmlSchema();
				xmlSchema.TargetNamespace = ((ns == null || ns.Length == 0) ? null : ns);
				xmlSchema.ElementFormDefault = XmlSchemaForm.Qualified;
				schemas.Add(xmlSchema);
			}
			xmlSchema.Items.Add(item);
		}
		if (referencingNs != null)
		{
			AddSchemaImport(ns, referencingNs);
		}
	}

	private void AddSchemaImport(string ns, string referencingNs)
	{
		if (referencingNs != null && ns != null && !(ns == referencingNs))
		{
			XmlSchema xmlSchema = schemas[referencingNs];
			if (xmlSchema == null)
			{
				throw new InvalidOperationException(Res.GetString("Missing schema targetNamespace=\"{0}\".", referencingNs));
			}
			if (ns != null && ns.Length > 0 && FindImport(xmlSchema, ns) == null)
			{
				XmlSchemaImport xmlSchemaImport = new XmlSchemaImport();
				xmlSchemaImport.Namespace = ns;
				xmlSchema.Includes.Add(xmlSchemaImport);
			}
		}
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
				if (xmlSchemaImport.Namespace == ns)
				{
					return xmlSchemaImport;
				}
			}
		}
		return null;
	}

	private XmlQualifiedName ExportTypeMapping(TypeMapping mapping, string ns)
	{
		if (mapping is ArrayMapping)
		{
			return ExportArrayMapping((ArrayMapping)mapping, ns);
		}
		if (mapping is EnumMapping)
		{
			return ExportEnumMapping((EnumMapping)mapping, ns);
		}
		if (mapping is PrimitiveMapping)
		{
			PrimitiveMapping primitiveMapping = (PrimitiveMapping)mapping;
			if (primitiveMapping.TypeDesc.IsXsdType)
			{
				return ExportPrimitiveMapping(primitiveMapping);
			}
			return ExportNonXsdPrimitiveMapping(primitiveMapping, ns);
		}
		if (mapping is StructMapping)
		{
			return ExportStructMapping((StructMapping)mapping, ns);
		}
		if (mapping is NullableMapping)
		{
			return ExportTypeMapping(((NullableMapping)mapping).BaseMapping, ns);
		}
		if (mapping is MembersMapping)
		{
			return ExportMembersMapping((MembersMapping)mapping, ns);
		}
		throw new ArgumentException(Res.GetString("Internal error."), "mapping");
	}

	private XmlQualifiedName ExportNonXsdPrimitiveMapping(PrimitiveMapping mapping, string ns)
	{
		XmlSchemaType dataType = mapping.TypeDesc.DataType;
		if (!SchemaContainsItem(dataType, "http://microsoft.com/wsdl/types/"))
		{
			AddSchemaItem(dataType, "http://microsoft.com/wsdl/types/", ns);
		}
		else
		{
			AddSchemaImport("http://microsoft.com/wsdl/types/", ns);
		}
		return new XmlQualifiedName(mapping.TypeDesc.DataType.Name, "http://microsoft.com/wsdl/types/");
	}

	private XmlQualifiedName ExportPrimitiveMapping(PrimitiveMapping mapping)
	{
		return new XmlQualifiedName(mapping.TypeDesc.DataType.Name, "http://www.w3.org/2001/XMLSchema");
	}

	private XmlQualifiedName ExportArrayMapping(ArrayMapping mapping, string ns)
	{
		while (mapping.Next != null)
		{
			mapping = mapping.Next;
		}
		XmlSchemaComplexType xmlSchemaComplexType = (XmlSchemaComplexType)types[mapping];
		if (xmlSchemaComplexType == null)
		{
			CheckForDuplicateType(mapping.TypeName, mapping.Namespace);
			xmlSchemaComplexType = new XmlSchemaComplexType();
			xmlSchemaComplexType.Name = mapping.TypeName;
			types.Add(mapping, xmlSchemaComplexType);
			AddSchemaItem(xmlSchemaComplexType, mapping.Namespace, ns);
			AddSchemaImport("http://schemas.xmlsoap.org/soap/encoding/", mapping.Namespace);
			AddSchemaImport("http://schemas.xmlsoap.org/wsdl/", mapping.Namespace);
			XmlSchemaComplexContentRestriction xmlSchemaComplexContentRestriction = new XmlSchemaComplexContentRestriction();
			XmlQualifiedName xmlQualifiedName = ExportTypeMapping(mapping.Elements[0].Mapping, mapping.Namespace);
			if (xmlQualifiedName.IsEmpty)
			{
				xmlQualifiedName = new XmlQualifiedName("anyType", "http://www.w3.org/2001/XMLSchema");
			}
			XmlSchemaAttribute xmlSchemaAttribute = new XmlSchemaAttribute();
			xmlSchemaAttribute.RefName = ArrayTypeQName;
			XmlAttribute xmlAttribute = new XmlAttribute("wsdl", "arrayType", "http://schemas.xmlsoap.org/wsdl/", Document);
			xmlAttribute.Value = xmlQualifiedName.Namespace + ":" + xmlQualifiedName.Name + "[]";
			xmlSchemaAttribute.UnhandledAttributes = new XmlAttribute[1] { xmlAttribute };
			xmlSchemaComplexContentRestriction.Attributes.Add(xmlSchemaAttribute);
			xmlSchemaComplexContentRestriction.BaseTypeName = ArrayQName;
			XmlSchemaComplexContent xmlSchemaComplexContent = new XmlSchemaComplexContent();
			xmlSchemaComplexContent.Content = xmlSchemaComplexContentRestriction;
			xmlSchemaComplexType.ContentModel = xmlSchemaComplexContent;
			if (xmlQualifiedName.Namespace != "http://www.w3.org/2001/XMLSchema")
			{
				AddSchemaImport(xmlQualifiedName.Namespace, mapping.Namespace);
			}
		}
		else
		{
			AddSchemaImport(mapping.Namespace, ns);
		}
		return new XmlQualifiedName(mapping.TypeName, mapping.Namespace);
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

	private void ExportElementAccessor(XmlSchemaGroupBase group, ElementAccessor accessor, bool repeats, bool valueTypeOptional, string ns)
	{
		XmlSchemaElement xmlSchemaElement = new XmlSchemaElement();
		xmlSchemaElement.MinOccurs = ((!(repeats || valueTypeOptional)) ? 1 : 0);
		xmlSchemaElement.MaxOccurs = (repeats ? decimal.MaxValue : 1m);
		xmlSchemaElement.Name = accessor.Name;
		xmlSchemaElement.IsNillable = accessor.IsNullable || accessor.Mapping is NullableMapping;
		xmlSchemaElement.Form = XmlSchemaForm.Unqualified;
		xmlSchemaElement.SchemaTypeName = ExportTypeMapping(accessor.Mapping, accessor.Namespace);
		group.Items.Add(xmlSchemaElement);
	}

	private XmlQualifiedName ExportRootMapping(StructMapping mapping)
	{
		if (!exportedRoot)
		{
			exportedRoot = true;
			ExportDerivedMappings(mapping);
		}
		return new XmlQualifiedName("anyType", "http://www.w3.org/2001/XMLSchema");
	}

	private XmlQualifiedName ExportStructMapping(StructMapping mapping, string ns)
	{
		if (mapping.TypeDesc.IsRoot)
		{
			return ExportRootMapping(mapping);
		}
		XmlSchemaComplexType xmlSchemaComplexType = (XmlSchemaComplexType)types[mapping];
		if (xmlSchemaComplexType == null)
		{
			if (!mapping.IncludeInSchema)
			{
				throw new InvalidOperationException(Res.GetString("The type {0} may not be exported to a schema because the IncludeInSchema property of the SoapType attribute is 'false'.", mapping.TypeDesc.Name));
			}
			CheckForDuplicateType(mapping.TypeName, mapping.Namespace);
			xmlSchemaComplexType = new XmlSchemaComplexType();
			xmlSchemaComplexType.Name = mapping.TypeName;
			types.Add(mapping, xmlSchemaComplexType);
			AddSchemaItem(xmlSchemaComplexType, mapping.Namespace, ns);
			xmlSchemaComplexType.IsAbstract = mapping.TypeDesc.IsAbstract;
			if (mapping.BaseMapping != null && mapping.BaseMapping.IncludeInSchema)
			{
				XmlSchemaComplexContentExtension xmlSchemaComplexContentExtension = new XmlSchemaComplexContentExtension();
				xmlSchemaComplexContentExtension.BaseTypeName = ExportStructMapping(mapping.BaseMapping, mapping.Namespace);
				XmlSchemaComplexContent xmlSchemaComplexContent = new XmlSchemaComplexContent();
				xmlSchemaComplexContent.Content = xmlSchemaComplexContentExtension;
				xmlSchemaComplexType.ContentModel = xmlSchemaComplexContent;
			}
			ExportTypeMembers(xmlSchemaComplexType, mapping.Members, mapping.Namespace);
			ExportDerivedMappings(mapping);
		}
		else
		{
			AddSchemaImport(mapping.Namespace, ns);
		}
		return new XmlQualifiedName(xmlSchemaComplexType.Name, mapping.Namespace);
	}

	private XmlQualifiedName ExportMembersMapping(MembersMapping mapping, string ns)
	{
		XmlSchemaComplexType xmlSchemaComplexType = (XmlSchemaComplexType)types[mapping];
		if (xmlSchemaComplexType == null)
		{
			CheckForDuplicateType(mapping.TypeName, mapping.Namespace);
			xmlSchemaComplexType = new XmlSchemaComplexType();
			xmlSchemaComplexType.Name = mapping.TypeName;
			types.Add(mapping, xmlSchemaComplexType);
			AddSchemaItem(xmlSchemaComplexType, mapping.Namespace, ns);
			ExportTypeMembers(xmlSchemaComplexType, mapping.Members, mapping.Namespace);
		}
		else
		{
			AddSchemaImport(mapping.Namespace, ns);
		}
		return new XmlQualifiedName(xmlSchemaComplexType.Name, mapping.Namespace);
	}

	private void ExportTypeMembers(XmlSchemaComplexType type, MemberMapping[] members, string ns)
	{
		XmlSchemaGroupBase xmlSchemaGroupBase = new XmlSchemaSequence();
		foreach (MemberMapping memberMapping in members)
		{
			if (memberMapping.Elements.Length != 0)
			{
				bool valueTypeOptional = memberMapping.CheckSpecified != 0 || memberMapping.CheckShouldPersist || !memberMapping.TypeDesc.IsValueType;
				ExportElementAccessors(xmlSchemaGroupBase, memberMapping.Elements, repeats: false, valueTypeOptional, ns);
			}
		}
		if (xmlSchemaGroupBase.Items.Count <= 0)
		{
			return;
		}
		if (type.ContentModel != null)
		{
			if (type.ContentModel.Content is XmlSchemaComplexContentExtension)
			{
				((XmlSchemaComplexContentExtension)type.ContentModel.Content).Particle = xmlSchemaGroupBase;
				return;
			}
			if (!(type.ContentModel.Content is XmlSchemaComplexContentRestriction))
			{
				throw new InvalidOperationException(Res.GetString("Invalid content {0}.", type.ContentModel.Content.GetType().Name));
			}
			((XmlSchemaComplexContentRestriction)type.ContentModel.Content).Particle = xmlSchemaGroupBase;
		}
		else
		{
			type.Particle = xmlSchemaGroupBase;
		}
	}

	private void ExportDerivedMappings(StructMapping mapping)
	{
		for (StructMapping structMapping = mapping.DerivedMappings; structMapping != null; structMapping = structMapping.NextDerivedMapping)
		{
			if (structMapping.IncludeInSchema)
			{
				ExportStructMapping(structMapping, mapping.TypeDesc.IsRoot ? null : mapping.Namespace);
			}
		}
	}

	private XmlQualifiedName ExportEnumMapping(EnumMapping mapping, string ns)
	{
		XmlSchemaSimpleType xmlSchemaSimpleType = (XmlSchemaSimpleType)types[mapping];
		if (xmlSchemaSimpleType == null)
		{
			CheckForDuplicateType(mapping.TypeName, mapping.Namespace);
			xmlSchemaSimpleType = new XmlSchemaSimpleType();
			xmlSchemaSimpleType.Name = mapping.TypeName;
			types.Add(mapping, xmlSchemaSimpleType);
			AddSchemaItem(xmlSchemaSimpleType, mapping.Namespace, ns);
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
		else
		{
			AddSchemaImport(mapping.Namespace, ns);
		}
		return new XmlQualifiedName(mapping.TypeName, mapping.Namespace);
	}
}

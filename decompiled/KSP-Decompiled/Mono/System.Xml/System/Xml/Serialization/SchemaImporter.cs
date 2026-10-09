using System.CodeDom.Compiler;
using System.Collections;
using System.Configuration;
using System.Security.Permissions;
using System.Xml.Serialization.Advanced;
using System.Xml.Serialization.Configuration;
using Microsoft.CSharp;
using Unity;

namespace System.Xml.Serialization;

[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
public abstract class SchemaImporter
{
	private XmlSchemas schemas;

	private StructMapping root;

	private CodeGenerationOptions options;

	private CodeDomProvider codeProvider;

	private TypeScope scope;

	private ImportContext context;

	private bool rootImported;

	private NameTable typesInUse;

	private NameTable groupsInUse;

	private SchemaImporterExtensionCollection extensions;

	internal ImportContext Context
	{
		get
		{
			if (context == null)
			{
				context = new ImportContext();
			}
			return context;
		}
	}

	internal CodeDomProvider CodeProvider
	{
		get
		{
			if (codeProvider == null)
			{
				codeProvider = new CSharpCodeProvider();
			}
			return codeProvider;
		}
	}

	public SchemaImporterExtensionCollection Extensions
	{
		get
		{
			if (extensions == null)
			{
				extensions = new SchemaImporterExtensionCollection();
			}
			return extensions;
		}
	}

	internal Hashtable ImportedElements => Context.Elements;

	internal Hashtable ImportedMappings => Context.Mappings;

	internal CodeIdentifiers TypeIdentifiers => Context.TypeIdentifiers;

	internal XmlSchemas Schemas
	{
		get
		{
			if (schemas == null)
			{
				schemas = new XmlSchemas();
			}
			return schemas;
		}
	}

	internal TypeScope Scope
	{
		get
		{
			if (scope == null)
			{
				scope = new TypeScope();
			}
			return scope;
		}
	}

	internal NameTable GroupsInUse
	{
		get
		{
			if (groupsInUse == null)
			{
				groupsInUse = new NameTable();
			}
			return groupsInUse;
		}
	}

	internal NameTable TypesInUse
	{
		get
		{
			if (typesInUse == null)
			{
				typesInUse = new NameTable();
			}
			return typesInUse;
		}
	}

	internal CodeGenerationOptions Options => options;

	internal SchemaImporter(XmlSchemas schemas, CodeGenerationOptions options, CodeDomProvider codeProvider, ImportContext context)
	{
		if (!schemas.Contains("http://www.w3.org/2001/XMLSchema"))
		{
			schemas.AddReference(XmlSchemas.XsdSchema);
			schemas.SchemaSet.Add(XmlSchemas.XsdSchema);
		}
		if (!schemas.Contains("http://www.w3.org/XML/1998/namespace"))
		{
			schemas.AddReference(XmlSchemas.XmlSchema);
			schemas.SchemaSet.Add(XmlSchemas.XmlSchema);
		}
		this.schemas = schemas;
		this.options = options;
		this.codeProvider = codeProvider;
		this.context = context;
		Schemas.SetCache(Context.Cache, Context.ShareTypes);
		if (System.Configuration.PrivilegedConfigurationManager.GetSection(ConfigurationStrings.SchemaImporterExtensionsSectionPath) is SchemaImporterExtensionsSection schemaImporterExtensionsSection)
		{
			extensions = schemaImporterExtensionsSection.SchemaImporterExtensionsInternal;
		}
		else
		{
			extensions = new SchemaImporterExtensionCollection();
		}
	}

	internal void MakeDerived(StructMapping structMapping, Type baseType, bool baseTypeCanBeIndirect)
	{
		structMapping.ReferencedByTopLevelElement = true;
		if (!(baseType != null))
		{
			return;
		}
		TypeDesc typeDesc = Scope.GetTypeDesc(baseType);
		if (typeDesc == null)
		{
			return;
		}
		TypeDesc typeDesc2 = structMapping.TypeDesc;
		if (baseTypeCanBeIndirect)
		{
			while (typeDesc2.BaseTypeDesc != null && typeDesc2.BaseTypeDesc != typeDesc)
			{
				typeDesc2 = typeDesc2.BaseTypeDesc;
			}
		}
		if (typeDesc2.BaseTypeDesc != null && typeDesc2.BaseTypeDesc != typeDesc)
		{
			throw new InvalidOperationException(Res.GetString("Type {0} cannot derive from {1} because it already has base type {2}.", structMapping.TypeDesc.FullName, baseType.FullName, typeDesc2.BaseTypeDesc.FullName));
		}
		typeDesc2.BaseTypeDesc = typeDesc;
	}

	internal string GenerateUniqueTypeName(string typeName)
	{
		typeName = CodeIdentifier.MakeValid(typeName);
		return TypeIdentifiers.AddUnique(typeName, typeName);
	}

	private StructMapping CreateRootMapping()
	{
		TypeDesc typeDesc = Scope.GetTypeDesc(typeof(object));
		return new StructMapping
		{
			TypeDesc = typeDesc,
			Members = new MemberMapping[0],
			IncludeInSchema = false,
			TypeName = "anyType",
			Namespace = "http://www.w3.org/2001/XMLSchema"
		};
	}

	internal StructMapping GetRootMapping()
	{
		if (root == null)
		{
			root = CreateRootMapping();
		}
		return root;
	}

	internal StructMapping ImportRootMapping()
	{
		if (!rootImported)
		{
			rootImported = true;
			ImportDerivedTypes(XmlQualifiedName.Empty);
		}
		return GetRootMapping();
	}

	internal abstract void ImportDerivedTypes(XmlQualifiedName baseName);

	internal void AddReference(XmlQualifiedName name, NameTable references, string error)
	{
		if (!(name.Namespace == "http://www.w3.org/2001/XMLSchema"))
		{
			if (references[name] != null)
			{
				throw new InvalidOperationException(Res.GetString(error, name.Name, name.Namespace));
			}
			references[name] = name;
		}
	}

	internal void RemoveReference(XmlQualifiedName name, NameTable references)
	{
		references[name] = null;
	}

	internal void AddReservedIdentifiersForDataBinding(CodeIdentifiers scope)
	{
		if ((options & CodeGenerationOptions.EnableDataBinding) != 0)
		{
			scope.AddReserved(CodeExporter.PropertyChangedEvent.Name);
			scope.AddReserved(CodeExporter.RaisePropertyChangedEventMethod.Name);
		}
	}

	internal SchemaImporter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

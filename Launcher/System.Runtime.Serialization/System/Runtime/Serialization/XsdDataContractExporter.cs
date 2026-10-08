using System.Collections.Generic;
using System.Reflection;
using System.Xml;
using System.Xml.Schema;

namespace System.Runtime.Serialization;

public class XsdDataContractExporter
{
	public ExportOptions Options
	{
		get
		{
			throw new NotImplementedException();
		}
		set
		{
			throw new NotImplementedException();
		}
	}

	public XmlSchemaSet Schemas
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public XsdDataContractExporter()
	{
		throw new NotImplementedException();
	}

	public XsdDataContractExporter(XmlSchemaSet schemas)
	{
		throw new NotImplementedException();
	}

	public bool CanExport(ICollection<Assembly> assemblies)
	{
		throw new NotImplementedException();
	}

	public bool CanExport(ICollection<Type> types)
	{
		throw new NotImplementedException();
	}

	public bool CanExport(Type type)
	{
		throw new NotImplementedException();
	}

	public void Export(ICollection<Assembly> assemblies)
	{
		throw new NotImplementedException();
	}

	public void Export(ICollection<Type> types)
	{
		throw new NotImplementedException();
	}

	public void Export(Type type)
	{
		throw new NotImplementedException();
	}

	public XmlQualifiedName GetRootElementName(Type type)
	{
		throw new NotImplementedException();
	}

	public XmlSchemaType GetSchemaType(Type type)
	{
		throw new NotImplementedException();
	}

	public XmlQualifiedName GetSchemaTypeName(Type type)
	{
		throw new NotImplementedException();
	}
}

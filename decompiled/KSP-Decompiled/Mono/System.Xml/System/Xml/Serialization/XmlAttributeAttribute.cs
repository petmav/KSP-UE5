using System.Xml.Schema;

namespace System.Xml.Serialization;

[AttributeUsage(AttributeTargets.Property | AttributeTargets.Field | AttributeTargets.Parameter | AttributeTargets.ReturnValue)]
public class XmlAttributeAttribute : Attribute
{
	private string attributeName;

	private Type type;

	private string ns;

	private string dataType;

	private XmlSchemaForm form;

	public Type Type
	{
		get
		{
			return type;
		}
		set
		{
			type = value;
		}
	}

	public string AttributeName
	{
		get
		{
			if (attributeName != null)
			{
				return attributeName;
			}
			return string.Empty;
		}
		set
		{
			attributeName = value;
		}
	}

	public string Namespace
	{
		get
		{
			return ns;
		}
		set
		{
			ns = value;
		}
	}

	public string DataType
	{
		get
		{
			if (dataType != null)
			{
				return dataType;
			}
			return string.Empty;
		}
		set
		{
			dataType = value;
		}
	}

	public XmlSchemaForm Form
	{
		get
		{
			return form;
		}
		set
		{
			form = value;
		}
	}

	public XmlAttributeAttribute()
	{
	}

	public XmlAttributeAttribute(string attributeName)
	{
		this.attributeName = attributeName;
	}

	public XmlAttributeAttribute(Type type)
	{
		this.type = type;
	}

	public XmlAttributeAttribute(string attributeName, Type type)
	{
		this.attributeName = attributeName;
		this.type = type;
	}
}

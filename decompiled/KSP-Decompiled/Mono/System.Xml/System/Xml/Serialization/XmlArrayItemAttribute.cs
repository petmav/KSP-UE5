using System.Xml.Schema;

namespace System.Xml.Serialization;

[AttributeUsage(AttributeTargets.Property | AttributeTargets.Field | AttributeTargets.Parameter | AttributeTargets.ReturnValue, AllowMultiple = true)]
public class XmlArrayItemAttribute : Attribute
{
	private string elementName;

	private Type type;

	private string ns;

	private string dataType;

	private bool nullable;

	private bool nullableSpecified;

	private XmlSchemaForm form;

	private int nestingLevel;

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

	public string ElementName
	{
		get
		{
			if (elementName != null)
			{
				return elementName;
			}
			return string.Empty;
		}
		set
		{
			elementName = value;
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

	public int NestingLevel
	{
		get
		{
			return nestingLevel;
		}
		set
		{
			nestingLevel = value;
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

	public bool IsNullable
	{
		get
		{
			return nullable;
		}
		set
		{
			nullable = value;
			nullableSpecified = true;
		}
	}

	internal bool IsNullableSpecified => nullableSpecified;

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

	public XmlArrayItemAttribute()
	{
	}

	public XmlArrayItemAttribute(string elementName)
	{
		this.elementName = elementName;
	}

	public XmlArrayItemAttribute(Type type)
	{
		this.type = type;
	}

	public XmlArrayItemAttribute(string elementName, Type type)
	{
		this.elementName = elementName;
		this.type = type;
	}
}

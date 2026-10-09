using System.Xml.Schema;

namespace System.Xml.Serialization;

[AttributeUsage(AttributeTargets.Property | AttributeTargets.Field | AttributeTargets.Parameter | AttributeTargets.ReturnValue, AllowMultiple = false)]
public class XmlArrayAttribute : Attribute
{
	private string elementName;

	private string ns;

	private bool nullable;

	private XmlSchemaForm form;

	private int order = -1;

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

	public bool IsNullable
	{
		get
		{
			return nullable;
		}
		set
		{
			nullable = value;
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

	public int Order
	{
		get
		{
			return order;
		}
		set
		{
			if (value < 0)
			{
				throw new ArgumentException(Res.GetString("Negative values are prohibited."), "Order");
			}
			order = value;
		}
	}

	public XmlArrayAttribute()
	{
	}

	public XmlArrayAttribute(string elementName)
	{
		this.elementName = elementName;
	}
}

namespace System.Xml.Serialization;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Interface | AttributeTargets.ReturnValue)]
public class XmlRootAttribute : Attribute
{
	private string elementName;

	private string ns;

	private string dataType;

	private bool nullable = true;

	private bool nullableSpecified;

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

	internal string Key => ((ns == null) ? string.Empty : ns) + ":" + ElementName + ":" + nullable;

	public XmlRootAttribute()
	{
	}

	public XmlRootAttribute(string elementName)
	{
		this.elementName = elementName;
	}
}

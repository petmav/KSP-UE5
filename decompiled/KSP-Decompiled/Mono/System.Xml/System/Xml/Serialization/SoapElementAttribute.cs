namespace System.Xml.Serialization;

[AttributeUsage(AttributeTargets.Property | AttributeTargets.Field | AttributeTargets.Parameter | AttributeTargets.ReturnValue)]
public class SoapElementAttribute : Attribute
{
	private string elementName;

	private string dataType;

	private bool nullable;

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
		}
	}

	public SoapElementAttribute()
	{
	}

	public SoapElementAttribute(string elementName)
	{
		this.elementName = elementName;
	}
}

namespace System.Xml.Serialization;

[AttributeUsage(AttributeTargets.Property | AttributeTargets.Field | AttributeTargets.Parameter | AttributeTargets.ReturnValue)]
public class SoapAttributeAttribute : Attribute
{
	private string attributeName;

	private string ns;

	private string dataType;

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

	public SoapAttributeAttribute()
	{
	}

	public SoapAttributeAttribute(string attributeName)
	{
		this.attributeName = attributeName;
	}
}

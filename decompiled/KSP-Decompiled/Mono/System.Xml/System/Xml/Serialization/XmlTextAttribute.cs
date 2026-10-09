namespace System.Xml.Serialization;

[AttributeUsage(AttributeTargets.Property | AttributeTargets.Field | AttributeTargets.Parameter | AttributeTargets.ReturnValue)]
public class XmlTextAttribute : Attribute
{
	private Type type;

	private string dataType;

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

	public XmlTextAttribute()
	{
	}

	public XmlTextAttribute(Type type)
	{
		this.type = type;
	}
}

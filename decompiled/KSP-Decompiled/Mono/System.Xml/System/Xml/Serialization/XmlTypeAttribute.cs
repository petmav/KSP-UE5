namespace System.Xml.Serialization;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Interface)]
public class XmlTypeAttribute : Attribute
{
	private bool includeInSchema = true;

	private bool anonymousType;

	private string ns;

	private string typeName;

	public bool AnonymousType
	{
		get
		{
			return anonymousType;
		}
		set
		{
			anonymousType = value;
		}
	}

	public bool IncludeInSchema
	{
		get
		{
			return includeInSchema;
		}
		set
		{
			includeInSchema = value;
		}
	}

	public string TypeName
	{
		get
		{
			if (typeName != null)
			{
				return typeName;
			}
			return string.Empty;
		}
		set
		{
			typeName = value;
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

	public XmlTypeAttribute()
	{
	}

	public XmlTypeAttribute(string typeName)
	{
		this.typeName = typeName;
	}
}

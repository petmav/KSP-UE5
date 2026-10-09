namespace System.Xml.Serialization;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Interface)]
public class SoapTypeAttribute : Attribute
{
	private string ns;

	private string typeName;

	private bool includeInSchema = true;

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

	public SoapTypeAttribute()
	{
	}

	public SoapTypeAttribute(string typeName)
	{
		this.typeName = typeName;
	}

	public SoapTypeAttribute(string typeName, string ns)
	{
		this.typeName = typeName;
		this.ns = ns;
	}
}

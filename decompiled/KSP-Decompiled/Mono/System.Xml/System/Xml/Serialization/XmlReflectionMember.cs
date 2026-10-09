namespace System.Xml.Serialization;

public class XmlReflectionMember
{
	private string memberName;

	private Type type;

	private XmlAttributes xmlAttributes = new XmlAttributes();

	private SoapAttributes soapAttributes = new SoapAttributes();

	private bool isReturnValue;

	private bool overrideIsNullable;

	public Type MemberType
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

	public XmlAttributes XmlAttributes
	{
		get
		{
			return xmlAttributes;
		}
		set
		{
			xmlAttributes = value;
		}
	}

	public SoapAttributes SoapAttributes
	{
		get
		{
			return soapAttributes;
		}
		set
		{
			soapAttributes = value;
		}
	}

	public string MemberName
	{
		get
		{
			if (memberName != null)
			{
				return memberName;
			}
			return string.Empty;
		}
		set
		{
			memberName = value;
		}
	}

	public bool IsReturnValue
	{
		get
		{
			return isReturnValue;
		}
		set
		{
			isReturnValue = value;
		}
	}

	public bool OverrideIsNullable
	{
		get
		{
			return overrideIsNullable;
		}
		set
		{
			overrideIsNullable = value;
		}
	}
}

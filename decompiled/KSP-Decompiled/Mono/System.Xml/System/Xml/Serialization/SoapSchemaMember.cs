namespace System.Xml.Serialization;

public class SoapSchemaMember
{
	private string memberName;

	private XmlQualifiedName type = XmlQualifiedName.Empty;

	public XmlQualifiedName MemberType
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
}

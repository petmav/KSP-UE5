using System.Reflection;

namespace System.Xml.Serialization;

[AttributeUsage(AttributeTargets.Property | AttributeTargets.Field | AttributeTargets.Parameter | AttributeTargets.ReturnValue, AllowMultiple = false)]
public class XmlChoiceIdentifierAttribute : Attribute
{
	private string name;

	private MemberInfo memberInfo;

	public string MemberName
	{
		get
		{
			if (name != null)
			{
				return name;
			}
			return string.Empty;
		}
		set
		{
			name = value;
		}
	}

	internal MemberInfo MemberInfo
	{
		get
		{
			return memberInfo;
		}
		set
		{
			memberInfo = value;
		}
	}

	public XmlChoiceIdentifierAttribute()
	{
	}

	public XmlChoiceIdentifierAttribute(string name)
	{
		this.name = name;
	}
}

using System.Reflection;

namespace System.Xml.Serialization;

internal class ChoiceIdentifierAccessor : Accessor
{
	private string memberName;

	private string[] memberIds;

	private MemberInfo memberInfo;

	internal string MemberName
	{
		get
		{
			return memberName;
		}
		set
		{
			memberName = value;
		}
	}

	internal string[] MemberIds
	{
		get
		{
			return memberIds;
		}
		set
		{
			memberIds = value;
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
}

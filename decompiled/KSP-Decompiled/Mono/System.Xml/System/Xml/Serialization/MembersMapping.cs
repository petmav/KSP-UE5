namespace System.Xml.Serialization;

internal class MembersMapping : TypeMapping
{
	private MemberMapping[] members;

	private bool hasWrapperElement = true;

	private bool validateRpcWrapperElement;

	private bool writeAccessors = true;

	private MemberMapping xmlnsMember;

	internal MemberMapping[] Members
	{
		get
		{
			return members;
		}
		set
		{
			members = value;
		}
	}

	internal MemberMapping XmlnsMember
	{
		get
		{
			return xmlnsMember;
		}
		set
		{
			xmlnsMember = value;
		}
	}

	internal bool HasWrapperElement
	{
		get
		{
			return hasWrapperElement;
		}
		set
		{
			hasWrapperElement = value;
		}
	}

	internal bool ValidateRpcWrapperElement
	{
		get
		{
			return validateRpcWrapperElement;
		}
		set
		{
			validateRpcWrapperElement = value;
		}
	}

	internal bool WriteAccessors
	{
		get
		{
			return writeAccessors;
		}
		set
		{
			writeAccessors = value;
		}
	}
}

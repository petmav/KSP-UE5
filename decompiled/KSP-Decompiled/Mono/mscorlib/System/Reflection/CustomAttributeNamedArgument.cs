using System.Runtime.InteropServices;

namespace System.Reflection;

[Serializable]
[ComVisible(true)]
public struct CustomAttributeNamedArgument
{
	private CustomAttributeTypedArgument typedArgument;

	private MemberInfo memberInfo;

	public MemberInfo MemberInfo => memberInfo;

	public CustomAttributeTypedArgument TypedValue => typedArgument;

	public bool IsField => memberInfo.MemberType == MemberTypes.Field;

	public string MemberName => memberInfo.Name;

	public CustomAttributeNamedArgument(MemberInfo memberInfo, object value)
	{
		this.memberInfo = memberInfo;
		typedArgument = (CustomAttributeTypedArgument)value;
	}

	public CustomAttributeNamedArgument(MemberInfo memberInfo, CustomAttributeTypedArgument typedArgument)
	{
		this.memberInfo = memberInfo;
		this.typedArgument = typedArgument;
	}

	public override string ToString()
	{
		return memberInfo.Name + " = " + typedArgument.ToString();
	}

	public override bool Equals(object obj)
	{
		if (!(obj is CustomAttributeNamedArgument customAttributeNamedArgument))
		{
			return false;
		}
		if (customAttributeNamedArgument.memberInfo == memberInfo)
		{
			return typedArgument.Equals(customAttributeNamedArgument.typedArgument);
		}
		return false;
	}

	public override int GetHashCode()
	{
		return (memberInfo.GetHashCode() << 16) + typedArgument.GetHashCode();
	}

	public static bool operator ==(CustomAttributeNamedArgument left, CustomAttributeNamedArgument right)
	{
		return left.Equals(right);
	}

	public static bool operator !=(CustomAttributeNamedArgument left, CustomAttributeNamedArgument right)
	{
		return !left.Equals(right);
	}
}

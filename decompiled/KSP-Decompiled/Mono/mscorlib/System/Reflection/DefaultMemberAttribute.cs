using System.Runtime.InteropServices;

namespace System.Reflection;

[Serializable]
[AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Interface)]
[ComVisible(true)]
public sealed class DefaultMemberAttribute : Attribute
{
	private string m_memberName;

	public string MemberName => m_memberName;

	public DefaultMemberAttribute(string memberName)
	{
		m_memberName = memberName;
	}
}

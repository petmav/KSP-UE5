using System.Runtime.InteropServices;

namespace System.Reflection;

[AttributeUsage(AttributeTargets.Assembly, Inherited = false)]
[ComVisible(true)]
public sealed class AssemblyDefaultAliasAttribute : Attribute
{
	private string m_defaultAlias;

	public string DefaultAlias => m_defaultAlias;

	public AssemblyDefaultAliasAttribute(string defaultAlias)
	{
		m_defaultAlias = defaultAlias;
	}
}

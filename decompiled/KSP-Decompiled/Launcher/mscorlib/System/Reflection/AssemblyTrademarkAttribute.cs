using System.Runtime.InteropServices;

namespace System.Reflection;

[ComVisible(true)]
[AttributeUsage(AttributeTargets.Assembly, Inherited = false)]
public sealed class AssemblyTrademarkAttribute : Attribute
{
	private string m_trademark;

	public string Trademark => m_trademark;

	public AssemblyTrademarkAttribute(string trademark)
	{
		m_trademark = trademark;
	}
}

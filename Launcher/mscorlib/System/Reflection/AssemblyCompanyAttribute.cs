using System.Runtime.InteropServices;

namespace System.Reflection;

[ComVisible(true)]
[AttributeUsage(AttributeTargets.Assembly, Inherited = false)]
public sealed class AssemblyCompanyAttribute : Attribute
{
	private string m_company;

	public string Company => m_company;

	public AssemblyCompanyAttribute(string company)
	{
		m_company = company;
	}
}

using System.Runtime.InteropServices;

namespace System.Reflection;

[AttributeUsage(AttributeTargets.Assembly, Inherited = false)]
[ComVisible(true)]
public sealed class AssemblyCultureAttribute : Attribute
{
	private string m_culture;

	public string Culture => m_culture;

	public AssemblyCultureAttribute(string culture)
	{
		m_culture = culture;
	}
}

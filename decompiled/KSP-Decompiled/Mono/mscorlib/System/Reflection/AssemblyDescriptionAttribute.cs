using System.Runtime.InteropServices;

namespace System.Reflection;

[AttributeUsage(AttributeTargets.Assembly, Inherited = false)]
[ComVisible(true)]
public sealed class AssemblyDescriptionAttribute : Attribute
{
	private string m_description;

	public string Description => m_description;

	public AssemblyDescriptionAttribute(string description)
	{
		m_description = description;
	}
}

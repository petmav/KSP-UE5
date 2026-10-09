using System.Runtime.InteropServices;

namespace System.Reflection;

[AttributeUsage(AttributeTargets.Assembly, Inherited = false)]
[ComVisible(true)]
public sealed class AssemblyTitleAttribute : Attribute
{
	private string m_title;

	public string Title => m_title;

	public AssemblyTitleAttribute(string title)
	{
		m_title = title;
	}
}

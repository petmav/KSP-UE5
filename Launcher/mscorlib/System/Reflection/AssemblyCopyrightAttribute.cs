using System.Runtime.InteropServices;

namespace System.Reflection;

[ComVisible(true)]
[AttributeUsage(AttributeTargets.Assembly, Inherited = false)]
public sealed class AssemblyCopyrightAttribute : Attribute
{
	private string m_copyright;

	public string Copyright => m_copyright;

	public AssemblyCopyrightAttribute(string copyright)
	{
		m_copyright = copyright;
	}
}

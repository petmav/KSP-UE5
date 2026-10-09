using System.Runtime.InteropServices;

namespace System.Reflection;

[AttributeUsage(AttributeTargets.Assembly, Inherited = false)]
[ComVisible(true)]
public sealed class AssemblyCopyrightAttribute : Attribute
{
	private string m_copyright;

	public string Copyright => m_copyright;

	public AssemblyCopyrightAttribute(string copyright)
	{
		m_copyright = copyright;
	}
}

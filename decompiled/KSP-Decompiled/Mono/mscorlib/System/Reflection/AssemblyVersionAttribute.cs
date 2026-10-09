using System.Runtime.InteropServices;

namespace System.Reflection;

[AttributeUsage(AttributeTargets.Assembly, Inherited = false)]
[ComVisible(true)]
public sealed class AssemblyVersionAttribute : Attribute
{
	private string m_version;

	public string Version => m_version;

	public AssemblyVersionAttribute(string version)
	{
		m_version = version;
	}
}

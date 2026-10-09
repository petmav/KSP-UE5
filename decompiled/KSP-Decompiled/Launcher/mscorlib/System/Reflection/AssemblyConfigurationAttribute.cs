using System.Runtime.InteropServices;

namespace System.Reflection;

[AttributeUsage(AttributeTargets.Assembly, Inherited = false)]
[ComVisible(true)]
public sealed class AssemblyConfigurationAttribute : Attribute
{
	private string m_configuration;

	public string Configuration => m_configuration;

	public AssemblyConfigurationAttribute(string configuration)
	{
		m_configuration = configuration;
	}
}

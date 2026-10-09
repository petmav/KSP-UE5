using System.Runtime.InteropServices;

namespace System.Reflection;

[ComVisible(true)]
[AttributeUsage(AttributeTargets.Assembly, Inherited = false)]
public sealed class AssemblyConfigurationAttribute : Attribute
{
	private string m_configuration;

	public string Configuration => m_configuration;

	public AssemblyConfigurationAttribute(string configuration)
	{
		m_configuration = configuration;
	}
}

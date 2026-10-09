using System.Runtime.InteropServices;

namespace System.Reflection;

[AttributeUsage(AttributeTargets.Assembly, Inherited = false)]
[ComVisible(true)]
public sealed class AssemblyKeyFileAttribute : Attribute
{
	private string m_keyFile;

	public string KeyFile => m_keyFile;

	public AssemblyKeyFileAttribute(string keyFile)
	{
		m_keyFile = keyFile;
	}
}

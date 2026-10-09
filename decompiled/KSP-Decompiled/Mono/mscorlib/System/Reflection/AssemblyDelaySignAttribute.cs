using System.Runtime.InteropServices;

namespace System.Reflection;

[AttributeUsage(AttributeTargets.Assembly, Inherited = false)]
[ComVisible(true)]
public sealed class AssemblyDelaySignAttribute : Attribute
{
	private bool m_delaySign;

	public bool DelaySign => m_delaySign;

	public AssemblyDelaySignAttribute(bool delaySign)
	{
		m_delaySign = delaySign;
	}
}

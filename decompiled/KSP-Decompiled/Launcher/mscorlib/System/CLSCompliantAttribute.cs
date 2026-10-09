using System.Runtime.InteropServices;

namespace System;

[Serializable]
[ComVisible(true)]
[AttributeUsage(AttributeTargets.All, Inherited = true, AllowMultiple = false)]
public sealed class CLSCompliantAttribute : Attribute
{
	private bool m_compliant;

	public bool IsCompliant => m_compliant;

	public CLSCompliantAttribute(bool isCompliant)
	{
		m_compliant = isCompliant;
	}
}

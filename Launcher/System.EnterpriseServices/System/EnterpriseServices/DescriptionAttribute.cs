using System.Runtime.InteropServices;

namespace System.EnterpriseServices;

[AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Class | AttributeTargets.Method | AttributeTargets.Interface, Inherited = true)]
[ComVisible(false)]
public sealed class DescriptionAttribute : Attribute
{
	public DescriptionAttribute(string desc)
	{
	}
}

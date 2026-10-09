using System.Runtime.InteropServices;

namespace System.Reflection;

[AttributeUsage(AttributeTargets.Assembly, Inherited = false)]
[ComVisible(true)]
public sealed class AssemblyProductAttribute : Attribute
{
	private string m_product;

	public string Product => m_product;

	public AssemblyProductAttribute(string product)
	{
		m_product = product;
	}
}

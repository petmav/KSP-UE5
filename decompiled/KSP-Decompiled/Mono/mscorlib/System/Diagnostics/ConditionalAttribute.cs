using System.Runtime.InteropServices;

namespace System.Diagnostics;

[Serializable]
[AttributeUsage(AttributeTargets.Class | AttributeTargets.Method, AllowMultiple = true)]
[ComVisible(true)]
public sealed class ConditionalAttribute : Attribute
{
	private string m_conditionString;

	public string ConditionString => m_conditionString;

	public ConditionalAttribute(string conditionString)
	{
		m_conditionString = conditionString;
	}
}

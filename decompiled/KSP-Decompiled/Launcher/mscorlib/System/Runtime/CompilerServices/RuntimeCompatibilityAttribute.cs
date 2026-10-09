namespace System.Runtime.CompilerServices;

[Serializable]
[AttributeUsage(AttributeTargets.Assembly, Inherited = false, AllowMultiple = false)]
public sealed class RuntimeCompatibilityAttribute : Attribute
{
	private bool m_wrapNonExceptionThrows;

	public bool WrapNonExceptionThrows
	{
		get
		{
			return m_wrapNonExceptionThrows;
		}
		set
		{
			m_wrapNonExceptionThrows = value;
		}
	}
}

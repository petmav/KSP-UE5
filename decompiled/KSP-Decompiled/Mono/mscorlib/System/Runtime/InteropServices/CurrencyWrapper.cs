namespace System.Runtime.InteropServices;

[Serializable]
[ComVisible(true)]
public sealed class CurrencyWrapper
{
	private decimal m_WrappedObject;

	public decimal WrappedObject => m_WrappedObject;

	public CurrencyWrapper(decimal obj)
	{
		m_WrappedObject = obj;
	}

	public CurrencyWrapper(object obj)
	{
		if (!(obj is decimal))
		{
			throw new ArgumentException(Environment.GetResourceString("Object must be of type Decimal."), "obj");
		}
		m_WrappedObject = (decimal)obj;
	}
}

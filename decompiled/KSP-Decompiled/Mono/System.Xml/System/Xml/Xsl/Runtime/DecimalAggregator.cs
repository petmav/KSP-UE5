using System.ComponentModel;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct DecimalAggregator
{
	private decimal result;

	private int cnt;

	public decimal SumResult => result;

	public decimal AverageResult => result / (decimal)cnt;

	public decimal MinimumResult => result;

	public decimal MaximumResult => result;

	public bool IsEmpty => cnt == 0;

	public void Create()
	{
		cnt = 0;
	}

	public void Sum(decimal value)
	{
		if (cnt == 0)
		{
			result = value;
			cnt = 1;
		}
		else
		{
			result += value;
		}
	}

	public void Average(decimal value)
	{
		if (cnt == 0)
		{
			result = value;
		}
		else
		{
			result += value;
		}
		cnt++;
	}

	public void Minimum(decimal value)
	{
		if (cnt == 0 || value < result)
		{
			result = value;
		}
		cnt = 1;
	}

	public void Maximum(decimal value)
	{
		if (cnt == 0 || value > result)
		{
			result = value;
		}
		cnt = 1;
	}
}

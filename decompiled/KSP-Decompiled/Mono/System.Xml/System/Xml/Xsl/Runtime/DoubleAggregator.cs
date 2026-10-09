using System.ComponentModel;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct DoubleAggregator
{
	private double result;

	private int cnt;

	public double SumResult => result;

	public double AverageResult => result / (double)cnt;

	public double MinimumResult => result;

	public double MaximumResult => result;

	public bool IsEmpty => cnt == 0;

	public void Create()
	{
		cnt = 0;
	}

	public void Sum(double value)
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

	public void Average(double value)
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

	public void Minimum(double value)
	{
		if (cnt == 0 || value < result || double.IsNaN(value))
		{
			result = value;
		}
		cnt = 1;
	}

	public void Maximum(double value)
	{
		if (cnt == 0 || value > result || double.IsNaN(value))
		{
			result = value;
		}
		cnt = 1;
	}
}

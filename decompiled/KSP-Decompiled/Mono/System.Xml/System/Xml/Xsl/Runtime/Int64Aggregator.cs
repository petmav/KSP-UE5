using System.ComponentModel;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct Int64Aggregator
{
	private long result;

	private int cnt;

	public long SumResult => result;

	public long AverageResult => result / cnt;

	public long MinimumResult => result;

	public long MaximumResult => result;

	public bool IsEmpty => cnt == 0;

	public void Create()
	{
		cnt = 0;
	}

	public void Sum(long value)
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

	public void Average(long value)
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

	public void Minimum(long value)
	{
		if (cnt == 0 || value < result)
		{
			result = value;
		}
		cnt = 1;
	}

	public void Maximum(long value)
	{
		if (cnt == 0 || value > result)
		{
			result = value;
		}
		cnt = 1;
	}
}

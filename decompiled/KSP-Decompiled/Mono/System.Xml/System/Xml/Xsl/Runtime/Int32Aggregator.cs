using System.ComponentModel;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct Int32Aggregator
{
	private int result;

	private int cnt;

	public int SumResult => result;

	public int AverageResult => result / cnt;

	public int MinimumResult => result;

	public int MaximumResult => result;

	public bool IsEmpty => cnt == 0;

	public void Create()
	{
		cnt = 0;
	}

	public void Sum(int value)
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

	public void Average(int value)
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

	public void Minimum(int value)
	{
		if (cnt == 0 || value < result)
		{
			result = value;
		}
		cnt = 1;
	}

	public void Maximum(int value)
	{
		if (cnt == 0 || value > result)
		{
			result = value;
		}
		cnt = 1;
	}
}

namespace System.Net.Http.Headers;

public class RangeItemHeaderValue : ICloneable
{
	public long? From { get; private set; }

	public long? To { get; private set; }

	public RangeItemHeaderValue(long? from, long? to)
	{
		if (!from.HasValue && !to.HasValue)
		{
			throw new ArgumentException();
		}
		if (from.HasValue && to.HasValue && from > to)
		{
			throw new ArgumentOutOfRangeException("from");
		}
		if (from < 0)
		{
			throw new ArgumentOutOfRangeException("from");
		}
		if (to < 0)
		{
			throw new ArgumentOutOfRangeException("to");
		}
		From = from;
		To = to;
	}

	object ICloneable.Clone()
	{
		return MemberwiseClone();
	}

	public override bool Equals(object obj)
	{
		if (obj is RangeItemHeaderValue { From: var from } rangeItemHeaderValue && from == From)
		{
			return rangeItemHeaderValue.To == To;
		}
		return false;
	}

	public override int GetHashCode()
	{
		return From.GetHashCode() ^ To.GetHashCode();
	}

	public override string ToString()
	{
		if (!From.HasValue)
		{
			return "-" + To.Value;
		}
		if (!To.HasValue)
		{
			return From.Value + "-";
		}
		return From.Value + "-" + To.Value;
	}
}

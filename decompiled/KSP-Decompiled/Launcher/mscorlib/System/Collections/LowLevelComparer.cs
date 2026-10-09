namespace System.Collections;

internal sealed class LowLevelComparer : IComparer
{
	internal static readonly LowLevelComparer Default = new LowLevelComparer();

	private LowLevelComparer()
	{
	}

	public int Compare(object a, object b)
	{
		if (a == b)
		{
			return 0;
		}
		if (a == null)
		{
			return -1;
		}
		if (b == null)
		{
			return 1;
		}
		if (a is IComparable comparable)
		{
			return comparable.CompareTo(b);
		}
		if (b is IComparable comparable2)
		{
			return -comparable2.CompareTo(a);
		}
		throw new ArgumentException("At least one object must implement IComparable.");
	}
}

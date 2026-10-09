namespace System.Globalization;

[Serializable]
internal sealed class CultureAwareComparer : StringComparer
{
	internal const CompareOptions ValidCompareMaskOffFlags = ~(CompareOptions.IgnoreCase | CompareOptions.IgnoreNonSpace | CompareOptions.IgnoreSymbols | CompareOptions.IgnoreKanaType | CompareOptions.IgnoreWidth | CompareOptions.StringSort);

	private readonly CompareInfo _compareInfo;

	private readonly CompareOptions _options;

	internal CultureAwareComparer(CompareInfo compareInfo, CompareOptions options)
	{
		_compareInfo = compareInfo;
		_options = options;
	}

	public override int Compare(string x, string y)
	{
		if ((object)x == y)
		{
			return 0;
		}
		if (x == null)
		{
			return -1;
		}
		if (y == null)
		{
			return 1;
		}
		return _compareInfo.Compare(x, y, _options);
	}

	public override bool Equals(string x, string y)
	{
		if ((object)x == y)
		{
			return true;
		}
		if (x == null || y == null)
		{
			return false;
		}
		return _compareInfo.Compare(x, y, _options) == 0;
	}

	public override int GetHashCode(string obj)
	{
		if (obj == null)
		{
			throw new ArgumentNullException("obj");
		}
		return _compareInfo.GetHashCode(obj, _options & ~CompareOptions.StringSort);
	}

	public override bool Equals(object obj)
	{
		if (obj is CultureAwareComparer cultureAwareComparer && _options == cultureAwareComparer._options)
		{
			return _compareInfo.Equals(cultureAwareComparer._compareInfo);
		}
		return false;
	}

	public override int GetHashCode()
	{
		return _compareInfo.GetHashCode() ^ (int)(_options & (CompareOptions)2147483647);
	}
}

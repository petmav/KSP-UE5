using System.Globalization;
using System.Runtime.Serialization;

namespace System;

[Serializable]
internal sealed class CultureAwareComparer : StringComparer
{
	private CompareInfo _compareInfo;

	private bool _ignoreCase;

	[OptionalField]
	private CompareOptions _options;

	internal CultureAwareComparer(CultureInfo culture, bool ignoreCase)
	{
		_compareInfo = culture.CompareInfo;
		_ignoreCase = ignoreCase;
		_options = (ignoreCase ? CompareOptions.IgnoreCase : CompareOptions.None);
	}

	internal CultureAwareComparer(CompareInfo compareInfo, bool ignoreCase)
	{
		_compareInfo = compareInfo;
		_ignoreCase = ignoreCase;
		_options = (ignoreCase ? CompareOptions.IgnoreCase : CompareOptions.None);
	}

	internal CultureAwareComparer(CompareInfo compareInfo, CompareOptions options)
	{
		_compareInfo = compareInfo;
		_options = options;
		_ignoreCase = (options & CompareOptions.IgnoreCase) == CompareOptions.IgnoreCase || (options & CompareOptions.OrdinalIgnoreCase) == CompareOptions.OrdinalIgnoreCase;
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
		return _compareInfo.GetHashCodeOfString(obj, _options);
	}

	public override bool Equals(object obj)
	{
		if (!(obj is CultureAwareComparer cultureAwareComparer))
		{
			return false;
		}
		if (_ignoreCase == cultureAwareComparer._ignoreCase)
		{
			if (_compareInfo.Equals(cultureAwareComparer._compareInfo))
			{
				return _options == cultureAwareComparer._options;
			}
			return false;
		}
		return false;
	}

	public override int GetHashCode()
	{
		int hashCode = _compareInfo.GetHashCode();
		if (!_ignoreCase)
		{
			return hashCode;
		}
		return ~hashCode;
	}
}

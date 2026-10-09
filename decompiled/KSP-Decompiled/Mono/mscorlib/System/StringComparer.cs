using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.Runtime.InteropServices;

namespace System;

[Serializable]
[ComVisible(true)]
public abstract class StringComparer : IComparer, IEqualityComparer, IComparer<string>, IEqualityComparer<string>
{
	private static readonly StringComparer _invariantCulture = new CultureAwareComparer(CultureInfo.InvariantCulture, ignoreCase: false);

	private static readonly StringComparer _invariantCultureIgnoreCase = new CultureAwareComparer(CultureInfo.InvariantCulture, ignoreCase: true);

	private static readonly StringComparer _ordinal = new OrdinalComparer(ignoreCase: false);

	private static readonly StringComparer _ordinalIgnoreCase = new OrdinalComparer(ignoreCase: true);

	public static StringComparer InvariantCulture => _invariantCulture;

	public static StringComparer InvariantCultureIgnoreCase => _invariantCultureIgnoreCase;

	public static StringComparer CurrentCulture => new CultureAwareComparer(CultureInfo.CurrentCulture, ignoreCase: false);

	public static StringComparer CurrentCultureIgnoreCase => new CultureAwareComparer(CultureInfo.CurrentCulture, ignoreCase: true);

	public static StringComparer Ordinal => _ordinal;

	public static StringComparer OrdinalIgnoreCase => _ordinalIgnoreCase;

	public static StringComparer Create(CultureInfo culture, bool ignoreCase)
	{
		if (culture == null)
		{
			throw new ArgumentNullException("culture");
		}
		return new CultureAwareComparer(culture, ignoreCase);
	}

	public int Compare(object x, object y)
	{
		if (x == y)
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
		if (x is string x2 && y is string y2)
		{
			return Compare(x2, y2);
		}
		if (x is IComparable comparable)
		{
			return comparable.CompareTo(y);
		}
		throw new ArgumentException(Environment.GetResourceString("At least one object must implement IComparable."));
	}

	public new bool Equals(object x, object y)
	{
		if (x == y)
		{
			return true;
		}
		if (x == null || y == null)
		{
			return false;
		}
		if (x is string x2 && y is string y2)
		{
			return Equals(x2, y2);
		}
		return x.Equals(y);
	}

	public int GetHashCode(object obj)
	{
		if (obj == null)
		{
			throw new ArgumentNullException("obj");
		}
		if (obj is string obj2)
		{
			return GetHashCode(obj2);
		}
		return obj.GetHashCode();
	}

	public abstract int Compare(string x, string y);

	public abstract bool Equals(string x, string y);

	public abstract int GetHashCode(string obj);
}

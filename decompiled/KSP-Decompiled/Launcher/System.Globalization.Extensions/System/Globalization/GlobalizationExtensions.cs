namespace System.Globalization;

public static class GlobalizationExtensions
{
	public static StringComparer GetStringComparer(this CompareInfo compareInfo, CompareOptions options)
	{
		if (compareInfo == null)
		{
			throw new ArgumentNullException("compareInfo");
		}
		switch (options)
		{
		case CompareOptions.Ordinal:
			return StringComparer.Ordinal;
		case CompareOptions.OrdinalIgnoreCase:
			return StringComparer.OrdinalIgnoreCase;
		default:
			if (((uint)options & 0xDFFFFFE0u) != 0)
			{
				throw new ArgumentException("Value of flags is invalid.", "options");
			}
			return new CultureAwareComparer(compareInfo, options);
		}
	}
}

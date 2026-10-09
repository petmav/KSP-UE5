namespace System.Collections.Generic;

[Serializable]
internal sealed class NonRandomizedStringEqualityComparer : EqualityComparer<string>
{
	private static volatile IEqualityComparer<string> s_nonRandomizedComparer;

	internal new static IEqualityComparer<string> Default => s_nonRandomizedComparer ?? (s_nonRandomizedComparer = new NonRandomizedStringEqualityComparer());

	public sealed override bool Equals(string x, string y)
	{
		return string.Equals(x, y);
	}

	public sealed override int GetHashCode(string obj)
	{
		return obj?.GetLegacyNonRandomizedHashCode() ?? 0;
	}
}

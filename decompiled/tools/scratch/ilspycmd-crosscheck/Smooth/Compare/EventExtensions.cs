namespace Smooth.Compare
{
	public static class EventExtensions
	{
		public static string ToStringCached(this ComparerType comparerType)
		{
			switch (comparerType)
			{
			default:
				return "Unknown";
			case ComparerType.EqualityComparer:
				return "Equality Comparer";
			case ComparerType.Comparer:
				return "Sort Order Comparer";
			}
		}
	}
}

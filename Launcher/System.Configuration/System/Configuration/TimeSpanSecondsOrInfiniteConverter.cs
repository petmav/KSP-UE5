using Unity;

namespace System.Configuration;

public sealed class TimeSpanSecondsOrInfiniteConverter : TimeSpanSecondsConverter
{
	public TimeSpanSecondsOrInfiniteConverter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

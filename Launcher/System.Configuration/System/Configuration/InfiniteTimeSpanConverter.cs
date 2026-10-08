using Unity;

namespace System.Configuration;

public sealed class InfiniteTimeSpanConverter : ConfigurationConverterBase
{
	public InfiniteTimeSpanConverter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using Unity;

namespace System.Configuration;

public sealed class InfiniteIntConverter : ConfigurationConverterBase
{
	public InfiniteIntConverter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using System.ComponentModel;
using Unity;

namespace System.Configuration;

public abstract class ConfigurationConverterBase : TypeConverter
{
	protected ConfigurationConverterBase()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
